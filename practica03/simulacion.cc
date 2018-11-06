/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/object.h"
#include "ns3/global-value.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/random-variable-stream.h"
//#include "Observador.h"

using namespace ns3;
#define NCSMA 6
#define BUS_TASA "100Mb/s"
#define BUS_RETARDO "5us"
#define P2P_TASA "1000Mb/s"
#define P2P_RETARDO "1us"
#define ON_TIME 1
#define ON_RATE "500Kb/s"
#define OFF_TIME 1
#define PCKT_LEN 512

NS_LOG_COMPONENT_DEFINE ("practica03");

int main (int argc, char *argv[]) {

  Time::SetResolution (Time::US);
  // Parámetros del modelo
  // - de la red de área local
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue(true));
  uint32_t nCsma = NCSMA; // número de equipos en el bus
  DataRate busRate (BUS_TASA); // capacidad del bus
  Time busDelay (BUS_RETARDO); // retardo de propagación del bus
  // - del enlace punto a punto
  DataRate p2pRate(P2P_TASA);  // capacidad del enlace
  Time p2pDelay (P2P_RETARDO); // retardo de propagación del enlace
  // - parámetros de la fuente on/Off
  DataRate onRate (ON_RATE);   // tasa de paquetes
  uint32_t tamPqt (PCKT_LEN);  // tamaño del paquete
  double ton (ON_TIME);   // tiempo de actividad
  double toff (OFF_TIME); // tiempo de silencio
  Ptr<RandomVariableStream> onTime = CreateObject<ExponentialRandomVariable>();
  Ptr<RandomVariableStream> offTime = CreateObject<ExponentialRandomVariable>();

  // Configuramos parámetros pasados por línea de comandos
  CommandLine cmd;
  cmd.AddValue ("ton", "Tiempo de actividad", ton);
  cmd.AddValue ("toff", "Tiempo de silencio", toff);
  cmd.AddValue ("tamPqt", "Tamaño del paquete", tamPqt);
  cmd.AddValue ("tasaEnvio", "Tasa de bit en el estado activo", onRate);
  cmd.Parse (argc, argv);

  // Configuración del escenario
  // nodos que pertenecen al enlace punto a punto
  NodeContainer nodosP2P;
  nodosP2P.Create (2);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue(p2pRate));
  p2p.SetChannelAttribute ("Delay", TimeValue (busDelay));

  NetDeviceContainer p2pDevices;
  p2pDevices = p2p.Install (nodosP2P);

  // nodos que pertenecen a la red de área local
  NodeContainer nodosCsma;
  // como primer nodo añadimos el encaminador que proporciona acceso
  // al enlace punto a punto.
  nodosCsma.Add (nodosP2P.Get (1));
  nodosCsma.Create (nCsma-1);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue(busRate));
  csma.SetChannelAttribute ("Delay", TimeValue (busDelay));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (nodosCsma);

  // instalamos la pila TCP/IP en todos los nodos de la red local
  InternetStackHelper stack;
  stack.Install (nodosCsma);
  // y les asignamos direcciones
  Ipv4AddressHelper csmaAddress;
  csmaAddress.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces = csmaAddress.Assign (csmaDevices);

  // realizamos el mismo proceso para el nodo punto a punto restante
  stack.Install (nodosP2P.Get (0));

  Ipv4AddressHelper p2pAddress;
  p2pAddress.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces = p2pAddress.Assign (p2pDevices);

  // cálculo de rutas
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  /***************************************************/

  // Instalación de las aplicaciones
  // comunicación fuente on/off -> nodo aislado utiliza UDP
  uint16_t port = 9;
  // sumidero para recibir los paquetes en el nodo aislado
  PacketSinkHelper sink ("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny(), port)));
  ApplicationContainer sinkApp = sink.Install(nodosP2P.Get (0));
  sinkApp.Start(Seconds(0.0));
  sinkApp.Stop(Seconds(10.0));

  // cliente OnOff en uno de los equipos de la red de área local
  OnOffHelper clientes ("ns3::UdpSocketFactory", Address (InetSocketAddress (p2pInterfaces.GetAddress (0), port)));
  onTime->SetAttribute ("Mean", DoubleValue(ton));
  offTime->SetAttribute ("Mean", DoubleValue(toff));
  clientes.SetAttribute("OnTime", PointerValue(onTime));
  clientes.SetAttribute("OffTime", PointerValue(offTime));
  clientes.SetAttribute("DataRate", DataRateValue(onRate));
  clientes.SetAttribute("PacketSize", UintegerValue(tamPqt));
  ApplicationContainer onoffApp = clientes.Install(nodosCsma.Get (nCsma-1));
  onoffApp.Start(Seconds(0.0));
  onoffApp.Stop(Seconds(10.0));

  // Aquí empieza la parte de análisis
  // paquetes desde el punto de vista de la puerta de enlace de la LAN
  csma.EnablePcap ("p03_gw", csmaDevices.Get (0), true);

  /*
  Observador   servidor (csmaDevices.Get (nCsma-1)->GetObject<CsmaNetDevice> ());
  Observador * observadores[nCsma-1];
  for (uint32_t i = 0; i < nCsma-1 ; i++)
   observadores[i] = new Observador (csmaDevices.Get (i)->GetObject<CsmaNetDevice> ());
   */

  // Lanzamos la simulación
  Simulator::Run ();
  // Finalizamos el pro
  Simulator::Destroy ();

  return 0;
}
