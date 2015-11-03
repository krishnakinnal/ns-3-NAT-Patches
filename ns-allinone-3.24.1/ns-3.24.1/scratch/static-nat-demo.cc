#include<iostream>
#include<fstream>
#include"ns3/core-module.h"
#include"ns3/network-module.h"
#include"ns3/internet-module.h"
#include"ns3/point-to-point-module.h"
#include"ns3/applications-module.h"
#include"ns3/csma-module.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("StaticNAT");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);

LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

NodeContainer nodes,second;
nodes.Create(4);

second.Create(3);

InternetStackHelper internet;
internet.Install(nodes);
internet.Install(second);

NodeContainer n34=NodeContainer(nodes.Get(3),second.Get(2));

CsmaHelper csma1,csma2;
csma1.SetChannelAttribute("DataRate", DataRateValue (5000000));
csma1.SetChannelAttribute("Delay", TimeValue (MilliSeconds (2)));
csma2.SetChannelAttribute("DataRate", DataRateValue (5000000));
csma2.SetChannelAttribute("Delay", TimeValue (MilliSeconds (2)));


NetDeviceContainer devices1 = csma1.Install (nodes);
NetDeviceContainer devices2 = csma2.Install (second);

PointToPointHelper p2p;
p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

NetDeviceContainer d4d3 = p2p.Install (n34);

Ipv4AddressHelper address1;
address1.SetBase ("192.168.1.0", "255.255.255.0");

Ipv4AddressHelper address2;
address2.SetBase ("11.1.1.0", "255.255.255.0");

Ipv4AddressHelper address3;
address3.SetBase ("10.1.1.0", "255.255.255.0");

Ipv4InterfaceContainer firstInterfaces = address1.Assign (devices1);
Ipv4InterfaceContainer secondInterfaces = address2.Assign (devices2);
Ipv4InterfaceContainer thirdInterfaces = address3.Assign (d4d3);

Ipv4NatHelper natHelper;
Ptr<Ipv4Nat> nat = natHelper.Install (nodes.Get (3));

nat->SetInside (1);
nat->SetOutside (2);

Ipv4StaticNatRule rule(Ipv4Address ("192.168.1.1"),49153,
Ipv4Address ("10.1.1.100"),8080,0);
Ipv4StaticNatRule rule1(Ipv4Address ("192.168.1.2"),49153,
Ipv4Address ("10.1.1.150"),8080,0);

nat->AddStaticRule (rule);
nat->AddStaticRule (rule1);

UdpEchoServerHelper echoServer (9);
ApplicationContainer serverApps = echoServer.Install (second.Get (0));
serverApps.Start (Seconds (1.0));
serverApps.Stop (Seconds (10.0));

UdpEchoClientHelper echoClient (secondInterfaces.GetAddress (0), 9);
echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
echoClient.SetAttribute ("PacketSize", UintegerValue (512));

ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
clientApps.Start (Seconds (2.0));
clientApps.Stop (Seconds (10.0));
clientApps = echoClient.Install (nodes.Get (1));
clientApps.Start (Seconds (2.0));
clientApps.Stop (Seconds (10.0));
clientApps = echoClient.Install (nodes.Get (2));
clientApps.Start (Seconds (2.0));
clientApps.Stop (Seconds (10.0));

Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
p2p.EnablePcapAll ("ipv4-mynat", false);

Simulator::Run ();

Ptr<OutputStreamWrapper> natStream = Create<OutputStreamWrapper>
("nat.myrules", std::ios::out);
nat->PrintTable (natStream);

Simulator::Destroy ();
return 0;
}







