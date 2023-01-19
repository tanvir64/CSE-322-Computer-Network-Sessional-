import os
from xml.etree import ElementTree as ET 
import sys
et=ET.parse(sys.argv[1])

total_received_bits = 0
total_received_packets = 0
total_transmitted_packets = 0
total_lost_packets = 0
total_delay = 0.0
simulation_start = 0.0
simulation_stop = 0.0
total_flow = 0
per_flow_throughput = 0.0
per_flow_tp_squared = 0.0

packet = open("packetdrop.txt",'w');

for flow in et.findall("FlowStats/Flow"):

    total_received_bits += int(flow.get('rxBytes'))
    total_received_packets += int(flow.get('rxPackets'))
    total_transmitted_packets += int(flow.get('txPackets'))
    total_lost_packets += int(flow.get('lostPackets'))
    total_delay += float(flow.get('delaySum')[:-2])
    tx_start = float(flow.get('timeFirstRxPacket')[:-2])
    tx_stop = float(flow.get('timeLastRxPacket')[:-2])
    flow_id = int(flow.get('flowId'))
    dropped_packets = int(flow.get('txPackets'))-int(flow.get('rxPackets')) 
    per_flow_simulation_time = tx_stop - tx_start
    if per_flow_simulation_time > 0:
        curr_throughput = int(flow.get('rxPackets'))*1.0/(per_flow_simulation_time)
    else: 
        curr_throughput = 0
    per_flow_throughput += curr_throughput
    per_flow_tp_squared += curr_throughput**2
    total_flow += 1

    packet.write(str(flow_id)+' '+str(dropped_packets)+'\n')   

    if simulation_start == 0.0:
       simulation_start = tx_start

    elif tx_start < simulation_start:
        simulation_start = tx_start

    if tx_stop > simulation_stop:
        simulation_stop = tx_stop

packet.close();
print("\nThrougput Calculation : ")
print("Total Received bits = "+str(total_received_bits*8))
print("Receiving Packet Start time = %.3f sec" % (simulation_start*1e-9,))
print("Receiving Packet Stop time = %.3f sec" % (simulation_stop*1e-9,))

duration = (simulation_stop-simulation_start)*1e-9
print("Receiving Packet Duration time = %.3f sec" % (duration,))
print("Network Throughput = %.3f Mbps" % (total_received_bits*8/(duration*1e6),))

print("\nRatio Calculation : ")
print("Total Transmitted Packets = "+str(total_transmitted_packets))
print("Total Received Packets = "+str(total_received_packets))
print("Packet Delivery Ratio = %.3f " % (total_received_packets*1.0/total_transmitted_packets,))
print("Packet Drop Ratio = %.3f " % (total_lost_packets*1.0/total_transmitted_packets,))
print("Network Mean Delay = %.3f ms" % (total_delay*1e-6/total_received_packets,))
# print("Jain's Fairness Index = %.3f" % (per_flow_throughput**2/(total_flow*per_flow_tp_squared),))





