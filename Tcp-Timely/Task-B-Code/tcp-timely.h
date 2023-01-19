#ifndef TCPTIMELY_H
#define TCPTIMELY_H

#include "tcp-congestion-ops.h"

namespace ns3 {

class TcpSocketState;

class TcpTimely : public TcpNewReno
{
public:

  static TypeId GetTypeId (void);

  TcpTimely (void);

  TcpTimely (const TcpTimely& sock);
  ~TcpTimely (void);

  std::string GetName () const;

  void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);      

  void IncreaseRate(Ptr<TcpSocketState> tcb,double rtt);     

  //in case of new_rtt < low threshold, rate will be incremented by a certain value
  void AdditiveIncrement(Ptr<TcpSocketState> tcb); 

  //in case of new_rtt > high threshold, rate will be decremented by a certain value
  void MultiplicativeDecrement(Ptr<TcpSocketState> tcb,double rtt);

  //when rate depends on gradient
  void CalculateFromGradient(Ptr<TcpSocketState> tcb,double normalized_gradient);           

  Ptr<TcpCongestionOps> Fork ();

protected:
private:
  void EnableTimely (Ptr<TcpSocketState> tcb);
  void DisableTimely ();

private:

  double m_alpha;   //!<EWMA weight parameter
  double m_beta;    //!<multiplicative decrement factor                 
  uint32_t m_delta;   //!<additive increment step in Mbps               
  Time m_lowThreshold;    //!<RTT low threshold
  Time m_highThreshold;   //!<RTT high threshold    
  Time m_baseRtt;                    //!< Minimum of all Timely RTT measurements seen during connection
  Time m_minRtt;                     //!< Minimum of all RTT measurements within last RTT
  double m_rttDiffernce;    //!<difference between two consecutive rtt measurements
  uint32_t m_cntRtt;                 //!< Number of RTT measurements during last RTT
  bool m_doingTimelyNow;              //!< If true, do Timely for this RTT 
  double m_normalizedGradient;    //!<normalized gradient used to calculate data sending rate
  double m_sendingRate;   //!<data sending rate,adjusted according to traffic condition in the network
  double m_previousRtt;   //previous rtt used to calculate new rtt difference

};
} // namespace ns3

#endif // TCPVEGAS_H
