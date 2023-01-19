// 
// 
//     2001:f00d::
//    *   *   *   *
//    |   |   |   |      Csma        lowpan_right
//   n8  n7  n6  n5====== n0 =====n1   n2    n3    n4
//     lowpan_left    2001:cafe::  |    |     |    |
//                                 *    *     *    *
//                                    2001:f00e::

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include <ns3/lr-wpan-error-model.h>


using namespace ns3;

int main (int argc, char** argv) {
  // int nodes = 20;
  // int no_of_flow = 10;
  uint32_t simulationTime = 100;  
  uint32_t Csma_Nodes_count=1;  
  uint32_t lowpan_nodes = 26;
  uint32_t range = 400;
  uint32_t segmentSize = 150;
  std::string recovery = "ns3::TcpClassicRecovery";

  std::string tcpVariants = "TcpVegas";
  tcpVariants = "ns3::" + tcpVariants;
  
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName (recovery)));
  Config::SetDefault 
  ("ns3::TcpL4Protocol::SocketType", 
                      TypeIdValue (TypeId::LookupByName (tcpVariants)));

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (segmentSize));

  NodeContainer lowpan_left;
  lowpan_left.Create (lowpan_nodes);

  NodeContainer lowpan_right;
  lowpan_right.Create (lowpan_nodes);

  NodeContainer Csma_Nodes;
  Csma_Nodes.Create (Csma_Nodes_count);
  Csma_Nodes.Add (lowpan_left.Get (0));
  Csma_Nodes.Add (lowpan_right.Get (0));

  MobilityHelper mobility;
  // creating a channel with range propagation loss model  
  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (range));
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<RangePropagationLossModel> propModel = CreateObject<RangePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);
 
  LrWpanHelper lrWpanHelper;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (80),
                                 "DeltaY", DoubleValue (80),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));
                                              
  mobility.Install (lowpan_left);
  mobility.Install (lowpan_right);

  LrWpanHelper lrWpanHelper1;
  // setting the channel in helper
  lrWpanHelper1.SetChannel(channel);
  
  NetDeviceContainer lr_devices = lrWpanHelper.Install (lowpan_left);
  lrWpanHelper1.AssociateToPan (lr_devices, 0);

  LrWpanHelper lrWpanHelper2;
  // setting the channel in helper
  lrWpanHelper1.SetChannel(channel);
  NetDeviceContainer lr_devices1 = lrWpanHelper2.Install (lowpan_right);
  lrWpanHelper2.AssociateToPan (lr_devices1, 0);

  InternetStackHelper ISv6;
  ISv6.Install (lowpan_left);
  ISv6.Install (lowpan_right);
  ISv6.Install (Csma_Nodes.Get (0));


  SixLowPanHelper sixlowpanhelper1;
  NetDeviceContainer sl_devices = sixlowpanhelper1.Install (lr_devices);
  // sixlowpanhelper1.SetDeviceAttribute("DataRate",StringValue("100Kbps"));

  SixLowPanHelper sixlowpanhelper2;
  NetDeviceContainer sl_devices1 = sixlowpanhelper2.Install (lr_devices1);
  // sixlowpanhelper2.SetDeviceAttribute("DataRate",StringValue("100Kbps"));

  CsmaHelper ch;
  NetDeviceContainer csmaDevices = ch.Install (Csma_Nodes);  
  ch.SetChannelAttribute("DataRate",StringValue("100Kbps"));

  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2001:f00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces;
  lr_interfaces = ipv6.Assign (sl_devices);
  lr_interfaces.SetForwarding (0, true);
  lr_interfaces.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:cafe::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer c_interfaces;
  c_interfaces = ipv6.Assign (csmaDevices);
  c_interfaces.SetForwarding (1, true);
  c_interfaces.SetDefaultRouteInAllNodes (1);

  ipv6.SetBase (Ipv6Address ("2001:f00e::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces1;
  lr_interfaces1 = ipv6.Assign (sl_devices1);
  lr_interfaces1.SetForwarding (2, true);
  lr_interfaces1.SetDefaultRouteInAllNodes (2);

  for (uint32_t i = 0; i < sl_devices.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  for (uint32_t i = 0; i < sl_devices1.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices1.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  uint32_t ports = 9;

  // for(int j=0;j<no_of_flow;j++){
    for( uint32_t i=1; i<=lowpan_nodes-1; i++ ) {      
      OnOffHelper server ("ns3::TcpSocketFactory", Inet6SocketAddress (c_interfaces.GetAddress (0, 1), 
                                ports+i));
      server.SetAttribute ("PacketSize", UintegerValue (segmentSize));
      server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      server.SetAttribute ("DataRate", DataRateValue (DataRate ("100Kbps")));
      ApplicationContainer sourceApps = server.Install (lowpan_left.Get (i));
      sourceApps.Start (Seconds(0));
      sourceApps.Stop (Seconds(simulationTime));
      PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
      Inet6SocketAddress (Ipv6Address::GetAny (), ports+i));
      sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApps = sinkApp.Install (Csma_Nodes.Get(0));
      sinkApps.Start (Seconds (0.0));
      sinkApps.Stop (Seconds (simulationTime));      
      ports++;
    }
  // }
  
  
  // for(int j=0;j<no_of_flow;j++){
    for( uint32_t i=1; i<=lowpan_nodes-1; i++ ) {
      OnOffHelper server ("ns3::TcpSocketFactory", Inet6SocketAddress (c_interfaces.GetAddress (0, 1), 
                                ports+i));
      server.SetAttribute ("PacketSize", UintegerValue (segmentSize));
      server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      server.SetAttribute ("DataRate", DataRateValue (DataRate ("100Kbps")));
      ApplicationContainer sourceApps = server.Install (Csma_Nodes.Get (0));
      sourceApps.Start (Seconds(0));
      sourceApps.Stop (Seconds(simulationTime));

      PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
      Inet6SocketAddress (Ipv6Address::GetAny (), ports+i));
      sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApps = sinkApp.Install (lowpan_right.Get(i));
      sinkApps.Start (Seconds (10.0));
      sinkApps.Stop (Seconds (simulationTime));
      
      ports++;
    }
  // }
  


  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor=flowHelper.InstallAll();
  flowMonitor->CheckForLostPackets ();

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  flowMonitor->SerializeToXmlFile("./Project/lr.xml",false,true);

  Simulator::Destroy ();

  return 0;
}

