// Small wireless ad-hoc UDP example using AODV.
// API details may differ across NS-3 releases. This is a starting template,
// not a complete multi-experiment routing-protocol comparison.
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
using namespace ns3;
int main(int argc,char *argv[]) {
    uint32_t count=4; CommandLine cmd; cmd.AddValue("nodes","Number of Wi-Fi nodes",count); cmd.Parse(argc,argv);
    if(count<2) count=2;
    NodeContainer nodes; nodes.Create(count);
    YansWifiChannelHelper channel=YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy; phy.SetChannel(channel.Create());
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode",StringValue("DsssRate11Mbps"),
        "ControlMode",StringValue("DsssRate1Mbps"));
    WifiMacHelper mac; mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices=wifi.Install(phy,mac,nodes);
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX",DoubleValue(0.0),"MinY",DoubleValue(0.0),
        "DeltaX",DoubleValue(40.0),"DeltaY",DoubleValue(0.0),
        "GridWidth",UintegerValue(count),"LayoutType",StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    AodvHelper aodv;
    InternetStackHelper internet; internet.SetRoutingHelper(aodv); internet.Install(nodes);
    Ipv4AddressHelper ip; ip.SetBase("10.3.0.0","255.255.255.0"); auto iface=ip.Assign(devices);

    uint16_t port=7000;
    UdpServerHelper server(port); auto sa=server.Install(nodes.Get(count-1));
    sa.Start(Seconds(1)); sa.Stop(Seconds(15));
    UdpClientHelper client(iface.GetAddress(count-1),port);
    client.SetAttribute("MaxPackets",UintegerValue(10000));
    client.SetAttribute("Interval",TimeValue(MilliSeconds(20)));
    client.SetAttribute("PacketSize",UintegerValue(256));
    auto ca=client.Install(nodes.Get(0)); ca.Start(Seconds(3)); ca.Stop(Seconds(14));
    phy.EnablePcap("wireless-aodv",devices,true);
    FlowMonitorHelper fm; auto mon=fm.InstallAll();
    Simulator::Stop(Seconds(15)); Simulator::Run(); mon->CheckForLostPackets();
    for(auto const &kv:mon->GetFlowStats()) {
        auto s=kv.second; if(!s.txPackets) continue;
        std::cout<<"TX="<<s.txPackets<<" RX="<<s.rxPackets
                 <<" PDR="<<100.0*s.rxPackets/s.txPackets<<"%"
                 <<" avg delay(ms)="<<(s.rxPackets?1000.0*s.delaySum.GetSeconds()/s.rxPackets:0)
                 <<" lost="<<s.lostPackets<<"\n";
    }
    mon->SerializeToXmlFile("wireless-flowmon.xml",true,true);
    Simulator::Destroy();
}
