/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/object.h"
#include "ns3/global-value.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/gnuplot.h"
#include "Observador.h"

using namespace ns3;
#define NCSMA     2
#define RETARDO   "5us"
#define TASA      "100Mb/s"
#define TAMANO    1204
#define INTERVALO "500ms"
#define STOP      "3s"
#define RUN       "1s"
#define ERROR     0

NS_LOG_COMPONENT_DEFINE ("practica02");

int simulacion(uint32_t nCsma, Time delay, DataRate rate, uint32_t packetSize, Time tstop, Time tstart, std::string interval, double perror) ;

int
main (int argc, char *argv[])
{
  NS_LOG_INFO("Inicio del modulo Simulacion");

  // Establecemos la resolución a microsegundos
  Time::SetResolution(Time::US);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue(true));
  uint32_t    nCsma = NCSMA;
  Time        delay (RETARDO);
  DataRate    rate (TASA);
  uint32_t    packetSize = TAMANO;
  Time        tstop (STOP);
  Time        tstart (RUN);
  std::string interval (INTERVALO);
  double      perror = ERROR;
  double      tasa_pares = 0;

  // Añadimos los parametros configurables por linea de comando
  CommandLine cmd = CommandLine();
  // Parámetros de la topología
  cmd.AddValue("nCsma","Numero de nodos de la red local", nCsma);
  cmd.AddValue("retardoProp","Retardo de propagación en el bus", delay);
  cmd.AddValue("capacidad","Capacidad del bus", rate);
  cmd.AddValue("perror","Probabilidad de error de bit", perror);
  // Parámetros de tráfico
  cmd.AddValue("tamPaquete","Tamaño de la SDU de aplicación", packetSize);
  cmd.AddValue("intervalo","Tiempo entre dos paquetes consecutivos enviados por el mismo cliente", interval);
  cmd.AddValue("tstart","Instante de simulación de inicio de las aplicaciones cliente", tstart);
  cmd.AddValue("tstop","Instante de simulación de fin de las aplicaciones clientes", tstop);

  cmd.Parse(argc, argv);
  // Debemos garantizar qu hay al menos un clientey el servidor.
  if (nCsma <= 1)
   nCsma = 2;



  Gnuplot2dDataset error_tasa ("Tasa de ecos cursados");
  error_tasa.SetStyle (Gnuplot2dDataset::LINES);
  error_tasa.SetErrorBars (Gnuplot2dDataset::Y);
  for (perror = 0; perror <= 0.5 ; perror+=0.01) {
    tasa_pares = simulacion(nCsma, delay, rate, packetSize, tstop, tstart, interval, perror);
    NS_LOG_INFO("Tasa de pares de eco: " << tasa_pares);
    error_tasa.Add (perror, tasa_pares, 0.9);
  }
  Gnuplot GraficaPoisson;
  GraficaPoisson.SetLegend ("Probabilidad de error de bit", "Tasa de pares solicitud-respuesta de eco correctamente cursados");
  GraficaPoisson.SetTitle ("Probabilidad de error frente a tasa de eco cursado");
  // Añado la curva a la gráfica
  GraficaPoisson.AddDataset (error_tasa);
  // Para terminar genero el fichero
  std::ofstream fichero_poisson ("practica02.plt");
  GraficaPoisson.GenerateOutput (fichero_poisson);
  fichero_poisson << "pause -1" << std::endl;
  fichero_poisson.close ();
  return 0;
}

int simulacion(uint32_t nCsma, Time delay, DataRate rate, uint32_t packetSize, Time tstop, Time tstart, std::string interval, double perror) {
  double retardosMedios[nCsma-1];
  double ecosRx = 0;
  double ecosTx = 0;
  // Aquí empieza el escenario
  NodeContainer csmaNodes;
  csmaNodes.Create (nCsma);
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue(rate));
  csma.SetChannelAttribute ("Delay", TimeValue (delay));
  Ptr<RateErrorModel> errores = CreateObject<RateErrorModel> ();
  errores->SetUnit (RateErrorModel::ERROR_UNIT_BIT);
  errores->SetRate (perror);
  csma.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (errores));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // Instalamos la pila TCP/IP en todos los nodos
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  // Y les asignamos direcciones
  Ipv4AddressHelper address;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces = address.Assign (csmaDevices);
  // Cálculo de rutas
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Configuramos el tráfico
  /////////// Instalación de las aplicaciones
  // Servidor en el último nodo
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApp = echoServer.Install (csmaNodes.Get (nCsma - 1));
  serverApp.Start (Seconds(0.0));
  serverApp.Stop (tstop);
  // Clientes: en todos los nodos menos en el último
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma - 1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10000));
  echoClient.SetAttribute ("Interval", TimeValue (Time (interval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  NodeContainer clientes;
  for (uint32_t i = 0; i < nCsma - 1; i++)
   clientes.Add (csmaNodes.Get (i));
  ApplicationContainer clientApps = echoClient.Install (clientes);
  clientApps.Start (tstart);
  clientApps.Stop (tstop);

  //Aquí empieza la parte de análisis
  // Paquetes desde el punto de vista del servidor
  csma.EnablePcap ("p02_servidor", csmaDevices.Get (nCsma - 1), true);


  Observador   servidor (csmaDevices.Get (nCsma-1)->GetObject<CsmaNetDevice> ());
  Observador * observadores[nCsma-1];
  for (uint32_t i = 0; i < nCsma-1 ; i++) {
    // Paquetes desde el punto de vista del cliente
    csma.EnablePcap ("p02_cliente", csmaDevices.Get (i), true);
    observadores[i] = new Observador (csmaDevices.Get (i)->GetObject<CsmaNetDevice> ());
    NS_LOG_INFO("Observador creado: " << observadores[i]);
  }
  // Lanzamos la simulación
  Simulator::Run ();
  // Calculamos tiempo medio de eco para todos los clientes
  for (uint32_t i = 0; i < nCsma-1 ; i++) {
    retardosMedios[i] = 0;
    retardosMedios[i] = observadores[i]->GetRetardoMedio();
    ecosRx += observadores[i]->GetEcosRx();
    ecosTx += observadores[i]->GetPaquetesTx();
    NS_LOG_INFO("Tiempo de eco: " << retardosMedios[i]);
    NS_LOG_INFO("Ecos recibidos: " << ecosRx);
    NS_LOG_INFO("Ecos enviados: " << ecosTx);
  }
  // Finalizamos el pro
  Simulator::Destroy ();

  return ecosRx/ecosTx;
}
