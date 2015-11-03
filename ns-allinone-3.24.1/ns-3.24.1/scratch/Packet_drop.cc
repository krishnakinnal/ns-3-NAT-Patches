
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License version 2 as
+ * published by the Free Software Foundation;
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
+ */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-netfilter-hook.h"
#include "ns3/callback.h"
#include "ns3/ipv4-l3-protocol.h"
#include <fstream>
#include <string>

using namespace ns3;
using std::string;

NS_LOG_COMPONENT_DEFINE ("NetfilterExampleDrop");

// store drop value between different packets
Verdicts_t drop = NF_DROP;

uint32_t
HookDropEven(Hooks_t hook, Ptr<Packet> packet, Ptr<NetDevice> in,
               Ptr<NetDevice> out, ContinueCallback& ccb)
{
    NS_LOG_UNCOND("**********Process Dropping Hook***********");

    if(drop == NF_ACCEPT)
    {
     std::cout<<"HookDropEven function is called and decision is DROP\n";
    	drop = NF_DROP;
    }
 else {
     std::cout<<"HookDropEven function is called and decision is ACCEPT\n";
    	drop = NF_ACCEPT;
    }
// If you want to see the difference when we are not dropping, uncomment
// the line below
//    drop = NF_ACCEPT;
    NS_LOG_UNCOND("  > Remove = " << drop);
    return drop;
}

// gives an string value for the drop reason
string dropReason(Ipv4L3Protocol::DropReason reason){
	string returnValue = "---";
	switch (reason)
	{
		case Ipv4L3Protocol::DROP_TTL_EXPIRED:
				returnValue= "DROP_TTL_EXPIRED";
				break;
		case Ipv4L3Protocol::DROP_NO_ROUTE:
				returnValue= "DROP_NO_ROUTE";
				break;
		case Ipv4L3Protocol::DROP_BAD_CHECKSUM:
				returnValue= "DROP_BAD_CHECKSUM";
				break;
		case Ipv4L3Protocol::DROP_INTERFACE_DOWN:
				returnValue= "DROP_INTERFACE_DOWN";
				break;
		case Ipv4L3Protocol::DROP_ROUTE_ERROR:
				returnValue= "DROP_ROUTE_ERROR";
				break;
		case Ipv4L3Protocol::DROP_FRAGMENT_TIMEOUT:
				returnValue= "DROP_FRAGMENT_TIMEOUT";
				break;
		case Ipv4L3Protocol::DROP_NF_DROP:
				returnValue= "DROP_NF_DROP";
				break;
		default:
				NS_LOG_UNCOND("UNEXPECTED Drop value");
				break;
	}
	return returnValue;
}

// Trace call back function
void DropTrace (const Ipv4Header & header, Ptr<const Packet> packet,
		Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4,  uint32_t interface)
{
    NS_LOG_UNCOND("-------- Drop Trace --------");
	std::cout << "Dropped packet!! from " << header.GetSource() << " to "
			  << header.GetDestination() << ", Reason: " << dropReason(reason)
			  << std::endl;
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  uint16_t port = 9;

  // Desired topology:  n0 <----> n1 <-----> n2
  // n0 and n1 in first container, n1 and n2 in second

  NodeContainer first;
  first.Create (2);

  NodeContainer second;
  second.Add ( first.Get (1) );
  second.Create (1);


  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices1;
  devices1 = pointToPoint.Install (first);

  NetDeviceContainer devices2;
  devices2 = pointToPoint.Install (second);

  InternetStackHelper stack;
  stack.Install (first);
  stack.Install (second.Get (1));

  Ipv4AddressHelper address1;
  address1.SetBase ("192.168.1.0", "255.255.255.0");

  Ipv4AddressHelper address2;
  address2.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer firstInterfaces = address1.Assign (devices1);
  Ipv4InterfaceContainer secondInterfaces = address2.Assign (devices2);
  

  //Hook Registering on Node 1
  Ptr<Ipv4> ipv4=first.Get (1)->GetObject<Ipv4> ();
  std::cout << "==============Number of interfaces on node " << first.Get (1)->GetId() << ": " << ipv4->GetNInterfaces () << std::endl;
  
  Ptr<Ipv4L3Protocol> ipv4L3 = DynamicCast <Ipv4L3Protocol>(first.Get (1)->GetObject<Ipv4> ());
  Ptr <Ipv4Netfilter> netfilter = ipv4L3->GetNetfilter ();

  NetfilterHookCallback nodehook = MakeCallback (&HookDropEven);


  Ipv4NetfilterHook nfh = Ipv4NetfilterHook (1, NF_INET_PRE_ROUTING, NF_IP_PRI_FIRST , nodehook);
  
  netfilter->RegisterHook (nfh);

  UdpEchoServerHelper echoServer (port);

  ApplicationContainer serverApps = echoServer.Install (second.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (15.0));

  UdpEchoClientHelper echoClient (secondInterfaces.GetAddress (1), port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (512));

  ApplicationContainer clientApps = echoClient.Install (first.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (15.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  pointToPoint.EnablePcapAll ("netfilter_drop", false);

  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("test_trace_netfilter.tr"));
  pointToPoint.EnablePcapAll ("netfilter-drop", false);

//  ipv4L3-> TraceConnectWithoutContext ("Drop", MakeCallback(&DropTrace));
  first.Get (1)->GetObject<Ipv4> ()-> TraceConnectWithoutContext ("Drop", MakeCallback(&DropTrace));
//  first.Get (0)->GetObject<Ipv4> ()-> TraceConnectWithoutContext ("Drop", MakeCallback(&DropTrace));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}


