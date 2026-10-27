
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/propagation-module.h"

#include <fstream>
#include <iomanip>
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[])
{
    double speed = 1.0;
    std::string technology = "WiFi";
    std::string csvFile = "results.csv";

    CommandLine cmd(__FILE__);
    cmd.AddValue("speed", "Mobile node speed in m/s", speed);
    cmd.AddValue("technology", "WiFi or BluetoothApprox", technology);
    cmd.AddValue("csv", "CSV output file", csvFile);
    cmd.Parse(argc, argv);

    if (speed != 1.0 && speed != 5.0 && speed != 10.0)
    {
        NS_FATAL_ERROR("Speed must be 1, 5, or 10 m/s");
    }

    bool bluetoothApprox = (technology == "BluetoothApprox");
    if (technology != "WiFi" && !bluetoothApprox)
    {
        NS_FATAL_ERROR(
            "Use WiFi or BluetoothApprox. Zigbee requires the LR-WPAN model.");
    }

    NodeContainer nodes;
    nodes.Create(2);

    // Node 0: access point/gateway. Node 1: mobile client.
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.AddPropagationLoss(
        "ns3::LogDistancePropagationLossModel",
        "Exponent", DoubleValue(3.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // BluetoothApprox is explicitly an approximation, not Bluetooth PHY.
    phy.Set("TxPowerStart", DoubleValue(
        bluetoothApprox ? 0.0 : 16.0));
    phy.Set("TxPowerEnd", DoubleValue(
        bluetoothApprox ? 0.0 : 16.0));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("ErpOfdmRate6Mbps"),
        "ControlMode", StringValue("ErpOfdmRate6Mbps"));

    WifiMacHelper mac;
    Ssid ssid = Ssid("CampusNetwork");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer station = wifi.Install(
        phy, mac, NodeContainer(nodes.Get(1)));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer accessPoint = wifi.Install(
        phy, mac, NodeContainer(nodes.Get(0)));

    MobilityHelper apMobility;
    apMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");
    apMobility.Install(nodes.Get(0));
    nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(
        Vector(0.0, 0.0, 0.0));

    MobilityHelper mobile;
    mobile.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(5.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(1.0),
        "DeltaY", DoubleValue(1.0),
        "GridWidth", UintegerValue(1),
        "LayoutType", StringValue("RowFirst"));

    mobile.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)),
        "Speed", StringValue(
            "ns3::ConstantRandomVariable[Constant=1.0]"),
        "Mode", StringValue("Time"),
        "Time", TimeValue(Seconds(1.0)));

    mobile.Install(nodes.Get(1));

    Ptr<RandomWalk2dMobilityModel> mobility =
        nodes.Get(1)->GetObject<RandomWalk2dMobilityModel>();
    mobility->SetAttribute(
        "Speed", StringValue(
            "ns3::ConstantRandomVariable[Constant=1.0]"));

    // Set the requested speed through the command-line parameter.
    mobility->SetAttribute(
        "Speed", StringValue(
            "ns3::ConstantRandomVariable[Constant=" +
            std::to_string(speed) + "]"));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces =
        address.Assign(NetDeviceContainer(station, accessPoint));

    uint16_t port = 9000;
    UdpServerHelper server(port);
    ApplicationContainer serverApps =
        server.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(19.0));

    UdpClientHelper client(interfaces.GetAddress(1), port);
    client.SetAttribute("MaxPackets", UintegerValue(100000));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
    client.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps =
        client.Install(nodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(18.0));

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    phy.EnablePcap("campus-wifi", station.Get(0), true);

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    FlowMonitor::FlowStatsContainer stats =
        monitor->GetFlowStats();

    std::ofstream out(csvFile, std::ios::app);
    out << std::fixed << std::setprecision(6);

    for (const auto &entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(entry.first);

        if (tuple.destinationPort != port)
        {
            continue;
        }

        const FlowMonitor::FlowStats &s = entry.second;
        double duration = s.timeLastRxPacket.GetSeconds()
                        - s.timeFirstTxPacket.GetSeconds();

        double throughput = duration > 0
            ? (s.rxBytes * 8.0 / duration / 1e6) : 0.0;

        double lossPercent = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets
            : 0.0;

        double avgDelayMs = s.rxPackets > 0
            ? 1000.0 * s.delaySum.GetSeconds() / s.rxPackets
            : 0.0;

        out << technology << ","
            << speed << ","
            << s.txPackets << ","
            << s.rxPackets << ","
            << throughput << ","
            << lossPercent << ","
            << avgDelayMs << ","
            << "NA"
            << "\n";

        std::cout << "\nTechnology: " << technology
                  << "\nSpeed: " << speed << " m/s"
                  << "\nSent packets: " << s.txPackets
                  << "\nReceived packets: " << s.rxPackets
                  << "\nThroughput: " << throughput << " Mbps"
                  << "\nPacket loss: " << lossPercent << "%"
                  << "\nAverage delay: " << avgDelayMs << " ms"
                  << "\nReceived power: NA (not measured by FlowMonitor)"
                  << "\n";
    }

    out.close();
    Simulator::Destroy();
    return 0;
}
