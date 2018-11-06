#include "Observador.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Observador");

Observador::Observador (Ptr<CsmaNetDevice> device_ptr)
{
  m_numTramasTx=0;
  m_miDevice_ptr=device_ptr;
  m_tiradastx=0;
  m_tiradasrx=0;
  ProgramaTrazas ();
}

void
Observador::ProgramaTrazas ()
{
   m_miDevice_ptr->TraceConnectWithoutContext ("PhyTxDrop", MakeCallback (&Observador::TiraTramaTx, this));
   m_miDevice_ptr->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&Observador::TiraTramaRx, this));
   /* m_miDevice_ptr->TraceConnectWithoutContext ("MacTx", MakeCallback (&Observador::PaqueteEntregado, this));
   m_miDevice_ptr->TraceConnectWithoutContext ("MacRx", MakeCallback (&Observador::PaqueteRecibido, this)); */
   m_miDevice_ptr->GetNode()->GetApplication(0)->TraceConnectWithoutContext ("Tx", MakeCallback (&Observador::PaqueteEntregado, this));
   m_miDevice_ptr->GetNode()->GetApplication(0)->TraceConnectWithoutContext ("Rx", MakeCallback (&Observador::PaqueteRecibido, this));
}

void
Observador::TiraTramaTx (Ptr<const Packet>pqt_ptr)
{
  m_tiradastx++;
}

void
Observador::TiraTramaRx (Ptr<const Packet>pqt_ptr)
{
  m_tiradasrx++;
}

void
Observador::PaqueteEntregado (Ptr<const Packet> pqt)
{
  NS_LOG_INFO("Paquete entregado por: " << this);
  m_instanteRxAp=Simulator::Now ();
  NS_LOG_INFO("Tiempo entregado: " << m_instanteRxAp);
  m_numTramasTx++;
}

void
Observador::PaqueteRecibido (Ptr<const Packet> pqt)
{
  NS_LOG_INFO("Paquete recibido por: " << this);
  NS_LOG_INFO("Tiempo recibido: " << Simulator::Now() << ", tiempo entregado: " << m_instanteRxAp);
  Time retardo=Simulator::Now ()-m_instanteRxAp;
  NS_LOG_INFO("Retardo: " << retardo);
  if (retardo > 0)
    m_retardos.Update (retardo.GetDouble ());
}

uint64_t
Observador::GetNumTramas ()
{
  return m_numTramasTx;
}

double Observador::GetRetardoMedio ()
{
  return m_retardos.Mean ();
}

double Observador::GetEcosRx ()
{
  return m_retardos.Count ();
}

double Observador::GetPaquetesTx()
{
   return m_numTramasTx;
}

double Observador::GetPaquetesErroneos()
{
   return m_tiradasrx;
}
