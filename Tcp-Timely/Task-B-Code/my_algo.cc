//
//                        
//                        LAN 10.1.3.0
//    ======================================================
//   |     |     |     |     |     |     |     |     |     |    
//  n12   n13   n14   n15   n16   n17   n18   n19   n20   n0 -------------- n1   n2   n3   n4   n5   n6   n7   n8   n9   n10   n11
//                                                          point-to-point  |    |    |    |    |    |    |    |    |     |     |
//                                                              10.1.1.0    ====================================================
//                                                                                              
//                                                                                          LAN 10.1.2.0


#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/rtt-estimator.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/tcp-socket-base.h"

// NS_LOG_COMPONENT_DEFINE ("update");

using namespace ns3;

// std::ofstream goodput("./Project/goodput.txt");
Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */

void
CalculateGoodput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateGoodput);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 16384;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */  
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 10;                        /* Simulation time in seconds. */
  
  uint32_t nCsma = 10;
  int no_of_flow = 5;
  
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpDctcp"));
  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
 
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  NodeContainer csmaNodes_left;
  csmaNodes_left.Add (p2pNodes.Get (0));
  csmaNodes_left.Create (nCsma);

  NodeContainer csmaNodes_right;
  csmaNodes_right.Add (p2pNodes.Get (1));
  csmaNodes_right.Create (nCsma);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));  

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  CsmaHelper csma_left;
  csma_left.SetChannelAttribute ("DataRate", StringValue ("10Gbps"));
  csma_left.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(20)));

  CsmaHelper csma_right;
  csma_right.SetChannelAttribute ("DataRate", StringValue ("10Gbps"));
  csma_right.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(20)));

  NetDeviceContainer csmaDevices_left;
  csmaDevices_left = csma_left.Install (csmaNodes_left);

  NetDeviceContainer csmaDevices_right;
  csmaDevices_right = csma_right.Install (csmaNodes_right);

  InternetStackHelper stack;
//   stack.Install (p2pNodes);
  stack.Install (csmaNodes_left);
  stack.Install (csmaNodes_right);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces_left;
  csmaInterfaces_left = address.Assign (csmaDevices_left);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces_right;
  csmaInterfaces_right = address.Assign (csmaDevices_right);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  p2pDevices.Get(0)->SetAttribute("ReceiveErrorModel",PointerValue(em));

  /* Install TCP Receiver on the access point */
  for(int i = 0; i < no_of_flow; i++){
    csmaDevices_left.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    csmaDevices_right.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    
    TypeId tid;
    std::stringstream nodeId_right;
    std::stringstream nodeId_left;
    if(i%2 == 0)
      tid = TypeId::LookupByName ("ns3::TcpDctcp");
    else
      tid = TypeId::LookupByName ("ns3::TcpTimely");
    nodeId_right << csmaNodes_right.Get(i)->GetId ();
    std::string specificNode = "/NodeList/" + nodeId_right.str () + "/$ns3::TcpL4Protocol/SocketType";
    // std::cout<<nodeId_right.str()<<std::endl;
    Config::Set (specificNode, TypeIdValue (tid));

    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
    ApplicationContainer sinkApp = sinkHelper.Install (csmaNodes_left.Get(i)); 
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (csmaInterfaces_left.GetAddress (i), 9+i)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    //ApplicationContainer serverApp = server.Install (csmaNodes.Get(i+1)); // server node assign

    ApplicationContainer serverApp = server.Install (csmaNodes_right.Get(i)); // server node assign

    /* Start Applications */
    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
  }

  // Simulator::Schedule (Seconds (1.1), &CalculateGoodput);

  /* Flow Monitor */
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor=flowHelper.InstallAll();

  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

  /* Start Simulation */
  Simulator::Stop (Seconds (simulationTime + 1));  

  Simulator::Run ();
   /* Flow Monitor File  */
  flowMonitor->SerializeToXmlFile("stat.xml",false,true);  

  double averageGoodput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));
  std::cout << "Average Goodput: "<<averageGoodput<<"Mbit/s" <<std::endl;
   
  Simulator::Destroy ();

  return 0;
}
