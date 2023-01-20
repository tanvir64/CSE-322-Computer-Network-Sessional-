# CSE-322-Computer-Network-Sessional-
A contribution was made to the NS-3 network simulator, specifically in the form of an implemented module for the **TCP-Timely** congestion control algorithm. The algorithm is based on the measurement of round-trip time (RTT) and utilizes this information to adjust the sending rate of data packets in order to avoid network congestion.

The module, implemented in C++ programming language, includes the necessary logic and functions to simulate the behavior of the TCP-Timely algorithm within the NS-3 environment. This includes the **calculation of RTT**, **the adjustment of sending rate based on the RTT values** and **the reaction to network events such as packet loss**. The module also includes the necessary interfaces to interact with other components of the NS-3 simulator, such as the IP layer and the packet scheduler.

The implementation of this module allows for the evaluation of the TCP-Timely algorithm in a simulated network environment and provides insights into its performance and behavior under different network conditions. This can be useful for researchers and developers who are interested in developing and improving congestion control algorithms for TCP.

Overall, this contribution to the NS-3 network simulator enriches the simulation capabilities and research possibilities of the platform by adding a new congestion control algorithm to the list of supported protocols
