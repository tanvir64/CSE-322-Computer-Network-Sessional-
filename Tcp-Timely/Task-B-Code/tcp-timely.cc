#include "tcp-timely.h"
#include "tcp-socket-state.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpTimely");
NS_OBJECT_ENSURE_REGISTERED (TcpTimely);

TypeId
TcpTimely::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTimely")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpTimely> ()
    .SetGroupName ("Internet")
    .AddAttribute ("Alpha", "EWMA weight parameter",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&TcpTimely::m_alpha),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("Beta", "Multiplicative Decrement Factor",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&TcpTimely::m_beta),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("Delta", "Additive Increment Step",
                   UintegerValue (10),
                   MakeUintegerAccessor (&TcpTimely::m_delta),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Low_threshold", "Low Threshold for Timely",
                   TimeValue (MicroSeconds(50)),
                   MakeTimeAccessor (&TcpTimely::m_lowThreshold),
                   MakeTimeChecker ())
    .AddAttribute ("High_threshold", "High Threshold for timely",
                   TimeValue (MicroSeconds(500)),
                   MakeTimeAccessor (&TcpTimely::m_highThreshold),
                   MakeTimeChecker ())                 
  ;
  return tid;
}

TcpTimely::TcpTimely (void)
  : TcpNewReno (),
    m_alpha (0.8),
    m_beta (0.8),
    m_delta (10),
    m_lowThreshold (MicroSeconds(50)),
    m_highThreshold (MicroSeconds(500)),
    m_baseRtt (Time::Max ()),
    m_minRtt (Time::Max ()),
    m_rttDiffernce (0),
    m_cntRtt (0),
    m_doingTimelyNow (true),
    m_sendingRate (10),
    m_previousRtt (0)
{
  NS_LOG_FUNCTION (this);
}

TcpTimely::TcpTimely (const TcpTimely& sock)
  : TcpNewReno (sock),
    m_alpha (sock.m_alpha),
    m_beta (sock.m_beta),
    m_delta (sock.m_delta),
    m_lowThreshold (sock.m_lowThreshold),
    m_highThreshold (sock.m_highThreshold),
    m_baseRtt (sock.m_baseRtt),
    m_minRtt (sock.m_minRtt),
    m_rttDiffernce (sock.m_rttDiffernce),
    m_cntRtt (sock.m_cntRtt),
    m_doingTimelyNow (true),
    m_sendingRate (sock.m_sendingRate),
    m_previousRtt (sock.m_previousRtt)
    
{
  NS_LOG_FUNCTION (this);
}

TcpTimely::~TcpTimely (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
TcpTimely::Fork (void)
{
  return CopyObject<TcpTimely> (this);
}

void
TcpTimely::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

  if (rtt.IsZero ())
    {
      return;
    }


  tcb->m_pacing = true;
  tcb->m_paceInitialWindow = true;
  double latest_rtt = static_cast<double>(rtt.GetSeconds());
  IncreaseRate(tcb,latest_rtt);

  m_minRtt = std::min (m_minRtt, rtt);
  NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);

  m_baseRtt = std::min (m_baseRtt, rtt);
  NS_LOG_DEBUG ("Updated m_baseRtt = " << m_baseRtt);

  // Update RTT counter
  m_cntRtt++;
  NS_LOG_DEBUG ("Updated m_cntRtt = " << m_cntRtt);
}

void
TcpTimely::IncreaseRate(Ptr<TcpSocketState> tcb,
                     double new_rtt)
{        
  double new_rtt_difference = new_rtt - m_previousRtt;
  m_previousRtt = new_rtt;
  m_rttDiffernce = (1-m_alpha)*m_rttDiffernce + m_alpha*new_rtt_difference;
  m_normalizedGradient = m_rttDiffernce/static_cast<double>(m_minRtt.GetSeconds());  
  
  if(new_rtt < static_cast<double>(m_lowThreshold.GetSeconds())){
    AdditiveIncrement(tcb);
  }  
  else if(new_rtt > static_cast<double>(m_highThreshold.GetSeconds())){
    MultiplicativeDecrement(tcb,new_rtt);    
  }
  else{
    CalculateFromGradient(tcb,m_normalizedGradient);
  }
}

void
TcpTimely::AdditiveIncrement(Ptr<TcpSocketState> tcb)
{
  std::cout<< "Inside additive increment" << std::endl;
  m_sendingRate = m_sendingRate + (m_delta/1000);
  tcb->m_pacingRate = DataRate(m_sendingRate); 
}

void
TcpTimely::MultiplicativeDecrement(Ptr<TcpSocketState> tcb,double new_rtt)
{

  double ratio = 1 - (static_cast<double>(m_highThreshold.GetSeconds())/new_rtt);
   m_sendingRate = m_sendingRate * (1- (m_beta * ratio)); 
   tcb->m_pacingRate = DataRate(m_sendingRate); 
}

void
TcpTimely::CalculateFromGradient(Ptr<TcpSocketState> tcb,double normalized_gradient)
{
  uint32_t n = 5;
  if(normalized_gradient <= 0){
    m_sendingRate = m_sendingRate + static_cast<double>(m_delta * n);
    tcb->m_pacingRate = DataRate(m_sendingRate); 
  }
  else{
    m_sendingRate = m_sendingRate * (1 - (m_beta * normalized_gradient));
    tcb->m_pacingRate = DataRate(m_sendingRate); 
  }
}

void
TcpTimely::EnableTimely (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  m_doingTimelyNow = true;  
  m_cntRtt = 0;
  m_minRtt = Time::Max ();
}

void
TcpTimely::DisableTimely ()
{
  NS_LOG_FUNCTION (this);

  m_doingTimelyNow = false;
}

void
TcpTimely::CongestionStateSet (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
  if (newState == TcpSocketState::CA_OPEN)
    {
      EnableTimely (tcb);
    }
  else
    {
      DisableTimely ();
    }
}

std::string
TcpTimely::GetName () const
{
  return "TcpTimely";
}

} // namespace ns3
