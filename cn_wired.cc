// NS-3 wired TCP + UDP shared-bottleneck experiment.
// Copy to scratch/cn_wired.cc. API is intended for contemporary NS-3 releases.
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
using namespace ns3;

int main(int argc, char *argv[]) {
    double bw = 2.0;
    CommandLine cmd;
    cmd.AddValue("bw", "Bottleneck Mbps: 2, 5 or 10", bw);
    cmd.Parse(argc, argv);

    NodeContainer n; n.Create(4);
    NodeContainer r; r.Create(2);
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    access.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize",
                    QueueSizeValue(QueueSize("50p")));
    PointToPointHelper core;
    core.SetDeviceAttribute("DataRate",
        StringValue(std::to_string(bw) + "Mbps"));
    core.SetChannelAttribute("Delay", StringValue("10ms"));
    core.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize",
                  QueueSizeValue(QueueSize("50p")));

    NetDeviceContainer a0=access.Install(n.Get(0),r.Get(0));
    NetDeviceContainer a1=access.Install(n.Get(1),r.Get(0));
    NetDeviceContainer b0=access.Install(r.Get(1),n.Get(3));
    NetDeviceContainer b1=access.Install(r.Get(1),n.Get(2));
    NetDeviceContainer mid=core.Install(r.Get(0),r.Get(1));

    InternetStackHelper internet; internet.Install(n); internet.Install(r);
    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0","255.255.255.0"); auto i0=ip.Assign(a0);
    ip.SetBase("10.1.2.0","255.255.255.0"); ip.Assign(a1);
    ip.SetBase("10.1.3.0","255.255.255.0"); auto i3=ip.Assign(b0);
    ip.SetBase("10.1.4.0","255.255.255.0"); auto i2=ip.Assign(b1);
    ip.SetBase("10.1.5.0","255.255.255.0"); ip.Assign(mid);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t tp=5000;
    Address ta(InetSocketAddress(i3.GetAddress(1),tp));
    PacketSinkHelper sink("ns3::TcpSocketFactory",ta);
    auto sa=sink.Install(n.Get(3)); sa.Start(Seconds(0.5)); sa.Stop(Seconds(20));
    OnOffHelper src("ns3::TcpSocketFactory",ta);
    src.SetAttribute("DataRate",DataRateValue(DataRate("5Mbps")));
    src.SetAttribute("PacketSize",UintegerValue(1024));
    src.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    src.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    auto ca=src.Install(n.Get(0)); ca.Start(Seconds(1)); ca.Stop(Seconds(19));

    uint16_t up=6000;
    UdpServerHelper us(up);
    auto usa=us.Install(n.Get(2)); usa.Start(Seconds(0.5)); usa.Stop(Seconds(20));
    UdpClientHelper uc(i2.GetAddress(1),up);
    uc.SetAttribute("PacketSize",UintegerValue(512));
    uc.SetAttribute("Interval",TimeValue(Seconds(512.0*8/1000000.0)));
    uc.SetAttribute("MaxPackets",UintegerValue(1000000));
    auto uca=uc.Install(n.Get(1)); uca.Start(Seconds(1)); uca.Stop(Seconds(19));

    access.EnablePcapAll("wired-access");
    core.EnablePcapAll("wired-core");
    FlowMonitorHelper fm;
    Ptr<FlowMonitor> mon=fm.InstallAll();
    Simulator::Stop(Seconds(20)); Simulator::Run();
    mon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> cl=DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());
    std::cout << "\nBottleneck=" << bw << " Mbps\n";
    for (auto const &kv: mon->GetFlowStats()) {
        auto t=cl->FindFlow(kv.first); auto s=kv.second;
        if (!s.txPackets) continue;
        double pdr=100.0*s.rxPackets/s.txPackets;
        double loss=100.0*(s.txPackets-s.rxPackets)/s.txPackets;
        double delay=s.rxPackets ? 1000.0*s.delaySum.GetSeconds()/s.rxPackets : 0;
        double dur=(s.timeLastRxPacket-s.timeFirstRxPacket).GetSeconds();
        double thr=dur>0 ? s.rxBytes*8.0/dur/1e6 : 0;
        std::cout << "Flow " << kv.first << " protocol=" << unsigned(t.protocol)
                  << " TX=" << s.txPackets << " RX=" << s.rxPackets
                  << " PDR=" << pdr << "% Loss=" << loss << "%"
                  << " Delay=" << delay << " ms Throughput=" << thr << " Mbps\n";
    }
    mon->SerializeToXmlFile("wired-flowmon.xml",true,true);
    Simulator::Destroy();
}
