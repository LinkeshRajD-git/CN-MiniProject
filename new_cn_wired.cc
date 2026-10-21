#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

#include <iostream>
#include <iomanip>

using namespace ns3;

int main(int argc, char *argv[])
{
    // 1. Command-line parameter: bottleneck bandwidth
    double bw = 2.0;

    CommandLine cmd;
    cmd.AddValue("bw", "Bottleneck bandwidth in Mbps", bw);
    cmd.Parse(argc, argv);

    // 2. Create four end hosts and two routers
    NodeContainer hosts;
    hosts.Create(4);

    NodeContainer routers;
    routers.Create(2);

    // 3. Configure TCP congestion control before applications
    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        StringValue("ns3::TcpCubic"));

    Config::SetDefault(
        "ns3::TcpSocket::SegmentSize",
        UintegerValue(1024));

    Config::SetDefault(
        "ns3::TcpSocket::InitialCwnd",
        UintegerValue(10));

    Config::SetDefault(
        "ns3::TcpSocket::SndBufSize",
        UintegerValue(1 << 20));

    Config::SetDefault(
        "ns3::TcpSocket::RcvBufSize",
        UintegerValue(1 << 20));

    // 4. Configure access links: 10 Mbps, 2 ms, 50-packet queue
    PointToPointHelper access;

    access.SetDeviceAttribute(
        "DataRate", StringValue("10Mbps"));

    access.SetChannelAttribute(
        "Delay", StringValue("2ms"));

    access.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize", StringValue("50p"));

    // 5. Configure central bottleneck
    PointToPointHelper bottleneck;

    bottleneck.SetDeviceAttribute(
        "DataRate",
        StringValue(std::to_string(bw) + "Mbps"));

    bottleneck.SetChannelAttribute(
        "Delay", StringValue("10ms"));

    bottleneck.SetQueue(
        "ns3::DropTailQueue<Packet>",
        "MaxSize", StringValue("50p"));

    // 6. Connect the five point-to-point links
    // Node 0 -> Router 1
    NetDeviceContainer dev0r1 =
        access.Install(hosts.Get(0), routers.Get(0));

    // Node 1 -> Router 1
    NetDeviceContainer dev1r1 =
        access.Install(hosts.Get(1), routers.Get(0));

    // Router 1 -> Router 2 (bottleneck)
    NetDeviceContainer devBtl =
        bottleneck.Install(routers.Get(0), routers.Get(1));

    // Router 2 -> Node 2
    NetDeviceContainer devR2N2 =
        access.Install(routers.Get(1), hosts.Get(2));

    // Router 2 -> Node 3
    NetDeviceContainer devR2N3 =
        access.Install(routers.Get(1), hosts.Get(3));

    // 7. Remove default root queue-discs so that the
    // configured DropTail device queues are exposed.
    TrafficControlHelper tch;

    tch.Uninstall(dev0r1);
    tch.Uninstall(dev1r1);
    tch.Uninstall(devBtl);
    tch.Uninstall(devR2N2);
    tch.Uninstall(devR2N3);

    // 8. Install Internet stack on hosts and routers
    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(routers);

    // 9. Assign a separate subnet to every link
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface0r1 =
        ipv4.Assign(dev0r1);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iface1r1 =
        ipv4.Assign(dev1r1);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceBtl =
        ipv4.Assign(devBtl);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceR2N2 =
        ipv4.Assign(devR2N2);

    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaceR2N3 =
        ipv4.Assign(devR2N3);

    // 10. Populate multi-hop routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 11. TCP flow: Node 0 -> Node 3, port 8080
    uint16_t tcpPort = 8080;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer tcpSinkApp =
        tcpSink.Install(hosts.Get(3));

    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(20.0));

    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(ifaceR2N3.GetAddress(1), tcpPort));

    tcpSource.SetAttribute(
        "SendSize", UintegerValue(1024));

    // Zero means unlimited data
    tcpSource.SetAttribute(
        "MaxBytes", UintegerValue(0));

    ApplicationContainer tcpSourceApp =
        tcpSource.Install(hosts.Get(0));

    tcpSourceApp.Start(Seconds(1.0));
    tcpSourceApp.Stop(Seconds(19.0));

    // 12. UDP flow: Node 1 -> Node 2, port 5000
    uint16_t udpPort = 5000;

    UdpServerHelper udpServer(udpPort);

    ApplicationContainer udpServerApp =
        udpServer.Install(hosts.Get(2));

    udpServerApp.Start(Seconds(0.0));
    udpServerApp.Stop(Seconds(20.0));

    UdpClientHelper udpClient(
        ifaceR2N2.GetAddress(1), udpPort);

    udpClient.SetAttribute(
        "MaxPackets", UintegerValue(0xFFFFFFFF));

    udpClient.SetAttribute(
        "PacketSize", UintegerValue(512));

    // 512 bytes * 8 bits / 1 Mbps = 0.004096 seconds
    udpClient.SetAttribute(
        "Interval",
        TimeValue(Seconds(512 * 8.0 / 1e6)));

    ApplicationContainer udpClientApp =
        udpClient.Install(hosts.Get(1));

    udpClientApp.Start(Seconds(1.0));
    udpClientApp.Stop(Seconds(19.0));

    // 13. Enable PCAP packet tracing
    access.EnablePcapAll("wired-access");
    bottleneck.EnablePcapAll("wired-bottleneck");

    // 14. Install FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> monitor = fmh.InstallAll();

    // 15. Run simulation
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    monitor->CheckForLostPackets();

    // 16. Read and display TCP/UDP flow statistics
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            fmh.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nBottleneck bandwidth: "
              << bw << " Mbps\n";

    for (const auto &kv : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(kv.first);

        const FlowMonitor::FlowStats &s = kv.second;

        // Display only the forward application flows
        if (t.destinationPort != tcpPort &&
            t.destinationPort != udpPort)
        {
            continue;
        }

        if (s.txPackets == 0)
            continue;

        double loss =
            100.0 * (s.txPackets - s.rxPackets)
            / s.txPackets;

        double pdr =
            100.0 * s.rxPackets / s.txPackets;

        double delay =
            s.rxPackets > 0
            ? (s.delaySum.GetSeconds()
               / s.rxPackets * 1000.0)
            : 0.0;

        double duration =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double throughput =
            duration > 0
            ? (s.rxBytes * 8.0 / duration / 1e6)
            : 0.0;

        std::cout << "\nFlow " << kv.first
                  << " | Protocol: "
                  << (t.destinationPort == tcpPort
                      ? "TCP" : "UDP")
                  << " | Port: " << t.destinationPort
                  << "\n  TX packets: " << s.txPackets
                  << "\n  RX packets: " << s.rxPackets
                  << "\n  PDR: " << pdr << "%"
                  << "\n  Packet loss: " << loss << "%"
                  << "\n  Average delay: " << delay << " ms"
                  << "\n  Throughput: " << throughput << " Mbps"
                  << "\n";
    }

    // 17. Save FlowMonitor results
    monitor->SerializeToXmlFile(
        "wired-flowmon.xml", true, true);

    Simulator::Destroy();

    return 0;
}
