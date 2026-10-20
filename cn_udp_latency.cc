// NS-3 UDP latency experiment with one or two flows over one P2P link.
// Run delay values 2, 5, 10, 20 ms and compare --flows=1 vs --flows=2.
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
using namespace ns3;
int main(int argc,char *argv[]) {
    double delayMs=2.0; uint32_t flows=1;
    CommandLine cmd; cmd.AddValue("delay","Link delay in ms",delayMs);
    cmd.AddValue("flows","Number of UDP flows (1 or 2)",flows); cmd.Parse(argc,argv);
    if (flows<1) flows=1; if (flows>2) flows=2;
    NodeContainer nodes; nodes.Create(2);
    PointToPointHelper p;
    p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
    p.SetChannelAttribute("Delay",StringValue(std::to_string(delayMs)+"ms"));
    p.SetQueue("ns3::DropTailQueue<Packet>","MaxSize",QueueSizeValue(QueueSize("50p")));
    auto dev=p.Install(nodes);
    InternetStackHelper internet; internet.Install(nodes);
    Ipv4AddressHelper ip; ip.SetBase("10.2.0.0","255.255.255.0");
    auto iface=ip.Assign(dev);

    for(uint16_t i=0;i<flows;i++) {
        uint16_t port=5000+i;
        UdpServerHelper server(port);
        auto sa=server.Install(nodes.Get(1)); sa.Start(Seconds(0.5)); sa.Stop(Seconds(10));
        UdpClientHelper client(iface.GetAddress(1),port);
        client.SetAttribute("PacketSize",UintegerValue(512));
        client.SetAttribute("Interval",TimeValue(Seconds(512.0*8/1000000.0)));
        client.SetAttribute("MaxPackets",UintegerValue(20000));
        auto ca=client.Install(nodes.Get(0)); ca.Start(Seconds(1)); ca.Stop(Seconds(9));
    }
    p.EnablePcapAll("udp-latency");
    FlowMonitorHelper fm; auto mon=fm.InstallAll();
    Simulator::Stop(Seconds(10)); Simulator::Run(); mon->CheckForLostPackets();
    auto cl=DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());
    std::cout<<"\nDelay="<<delayMs<<" ms, flows="<<flows<<"\n";
    for(auto const &kv:mon->GetFlowStats()) {
        auto t=cl->FindFlow(kv.first); auto s=kv.second;
        if(t.protocol!=17 || !s.txPackets) continue;
        double pdr=100.0*s.rxPackets/s.txPackets;
        double loss=100.0*(s.txPackets-s.rxPackets)/s.txPackets;
        double d=s.rxPackets?1000.0*s.delaySum.GetSeconds()/s.rxPackets:0;
        double dur=(s.timeLastRxPacket-s.timeFirstRxPacket).GetSeconds();
        double thr=dur>0?s.rxBytes*8.0/dur/1e6:0;
        std::cout<<"port "<<t.destinationPort<<": TX="<<s.txPackets<<" RX="<<s.rxPackets
                 <<" PDR="<<pdr<<"% loss="<<loss<<"% delay="<<d
                 <<"ms throughput="<<thr<<"Mbps\n";
    }
    Simulator::Destroy();
}
