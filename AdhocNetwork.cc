/*
  --------------Universidad Nacional De Colombia -  Sede bogota---------
  ------  Curso Modelos estocasticos y simulacion en computacion y comunicaciones
  ------- 17 DE JUNIO DEL 2020
  ----Autores:
  -Andres Duvan Chaves Mosquera
  -Duvan Alberto Andrade
  -Ivan Rene Delgado Gonzalez
  -Juan Pablo Giron Bastidas
 
 * El siguiente es un script utilizado en la simulación de red inalámbrica
 * una manet (mobile ad hoc network) con 30 nodos en total, 5 de los cuales
 * funcionan como sumideros de paquetes provenientes de tráfico de aplicación
 * on/off instalada en otros 5 nodos. También, se tiene 1 servidor UdpEchoServer
 * recibiendo peticiones Udp de otros 5 cinco nodos y reenviandolos.
 * La cantidad de paquetes y su tamaño puede variar, por defecto, los paquetes
 * son de 64 bits y la cantidad 150, esto para la aplicación UdpEchoClient,
 * mientras que para on/off la cantidad de paquetes es indefinida puesto
 * que se repite durante toda la simulación. Por defecto la simulación trabaja
 * con un poder de transmisión de señales de radio de 7dBm. Se utiliza el modelo
 * de perdida de señal de Friss y los nodos se distribuyen en un espacio 3D de 
 * 500mx500mx30m. Se combina la simulación con un agente de opengym, el cual
 * recibe inputs del ambiente de del script y toma acciones por medio de
 * actuadores para modificar configuraciones que favorezcan a la red manet.
 */

// Libreria para registrar componentes de Logging
#include "ns3/command-line.h"

// Librerias necesarias para la lógica del programa 
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"

// Librerias necesarias para la configuracion de un canal inalámbrico
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-channel.h"

// Libreria para configurar direccionamiento IPv4
#include "ns3/ipv4-address-helper.h"

// Libreria para configurar el modelo de movilidad 3d
#include "ns3/mobility-model.h"
#include "ns3/mobility-helper.h"

// Libreria del stack de protocolos de internet
#include "ns3/internet-stack-helper.h"

// Libreria del flow monitor para sacar estadisticas y resultados
#include "ns3/flow-monitor-helper.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"

//Protocolos de enrutamiento para una red manet
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"


#include "ns3/applications-module.h"
//Librerias para usar las abstracciones de OpenGy
#include "ns3/core-module.h"
#include "ns3/opengym-module.h"


using namespace ns3;

/*
 * Variables globales para mantener guardar algunos parametros de 
 * la simulacion y la red manet en su conjunto
*/

int NBYTESTOTAL = 0;
int NRECEIVEDPACKETS = 0;

NS_LOG_COMPONENT_DEFINE ("AdhocNetwork");


//Definicion del espacio de observacion de opengym, este lo utilizara para observar la ejecuc
Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  //Definicion de los parametros que definen el espacio de observacion sobre el cual se encuentra el agente
  uint32_t nodeNum = 30;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

//Definicion del espacio de acciones
Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  //Definicion de los parametros que definen el espacio de observacion sobre el cual se encuentra el agente
  uint32_t nodeNum = 30;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymDiscreteSpace> space = CreateObject<OpenGymDiscreteSpace> (nodeNum);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

//Definicion de la condicion de fin,la cual le indicara al agente cuando debe parar su ciclo
bool MyGetGameOver(void)
{

  bool isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == 10 && test) {
      isGameOver = true;
  }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

// El entorno de Gym abstrae su informacion de la forma en que esta funcion la envia, y basado en esta informacion hace su aprendizaje
Ptr<OpenGymDataContainer> MyGetObservation(void)
{
  uint32_t nodeNum = 30;
  uint32_t low = 0.0;
  uint32_t high = 10.0;
  Ptr<UniformRandomVariable> rngInt = CreateObject<UniformRandomVariable> ();

  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenGymBoxContainer<uint32_t> > box = CreateObject<OpenGymBoxContainer<uint32_t> >(shape);

  for (uint32_t i = 0; i<nodeNum; i++){
    uint32_t value = rngInt->GetInteger(low, high);
    box->AddValue(value);
  }

  NS_LOG_UNCOND ("MyGetObservation: " << box);
  return box;
}

// Esta funcion establece un criterio para abstraer la ganancia que ganamos con el agente de opengym
float MyGetReward(void)
{

  static float reward = 0.0;
  reward += 1;
  return reward;
}


//Este es un opcional de opengym, que nos deja hacer prints que puedan ser pertinentes para la simulacio
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}

// Metodo que ejecuta las acciones recibidas del agente
bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
  NS_LOG_UNCOND ("MyExecuteActions: " << action);
  return true;
}

//Cronograma que le indica a la simulacion cual es el paso siguiente a ejecutar
void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{
  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState();
}

/* Esta función define el comportamiento de la simulación al registrar un paquete entrante por medio de
 * socket, el cual contiene el paquete entrante y la dirección desde donde lo enviaron
 */
void
ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
    
      // Se aumentan estas variables globales a manera de generar estadísticas
      NBYTESTOTAL += packet->GetSize ();
      NRECEIVEDPACKETS += 1;
      
      std::ostringstream oss;
      
      // Se imprime el registro del paquete
      oss << "A los " << Simulator::Now ().GetSeconds () << " segundos, el Nodo " << socket->GetNode ()->GetId ();
      if (InetSocketAddress::IsMatchingType (senderAddress))
        {
        // Obtención de dirección de llegada
          InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
          oss << " recibió un paquete del origen " << addr.GetIpv4 ();
        }
      else
        {
          oss << " recibió un paquete";
        }

        // Logging del paquete en consola
      NS_LOG_UNCOND (oss.str());
    }
}

int main (int argc, char *argv[])
{
  // Cantidad de nodos que componen red adhoc
  int N_NODES = 30;

  // Tiempo en segundos que dura la simulación
  double stopTime = 150;

  // Poder de transmision de cada nodo (en dBm)
  double txp = 7;

  /* Tiempo de pausa de los nodos antes de iniciar movimiento en
   * modelo de movilidad RandomWaypoint (0s por defecto)
   */
  double nodePauseTime = 0;

  // Tipo de protocolo de enrutamiento 
  int protocolType = 1;
  
  // Nombre de protocolo de enrutamiento
  std::string protocolTypeName = "OLSR";

  double interval = 2.0; // seconds
  bool verbose = false;

  int maxPackets = 150;


  // Parameters of the scenario
  uint32_t simSeed = 1;
  double simulationTime = 1; //seconds
  double envStepTime = 0.1; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 5555;
  uint32_t testArg = 0;
  
  // Ratio de transmisión wifi
  std::string phyMode ("DsssRate11Mbps");
  std::string packetSize ("64"); // b
  CommandLine cmd;
  
  
   //Parametros que son posibles cambiar por consola
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("protocolType", "Tipo de protocolo de enrutamiento (1=OLSR, 2=AODV, 3=DSDV, 4=DSR)", protocolType);
  cmd.AddValue ("txp", "Poder de transmisión de señales", txp);
  cmd.AddValue ("packetSize", "Tamaño de paquete", packetSize);
  cmd.AddValue ("interval", "Intervalo entre paquetes", interval);
  cmd.AddValue ("maxPackets", "Cantidad de paquetes a enviar con UdpClient", maxPackets);
  cmd.AddValue ("stopTime", "Tiempo de simulación", stopTime);
  cmd.AddValue ("verbose", "permitir componenetes de logueo de WifiNetDevice", verbose);
  cmd.AddValue ("openGymPort", "Número de puerto para el ambiente de OpenGym. Defecto: 5555", openGymPort);
  cmd.AddValue ("simSeed", "Semilla para generador de aleatorios. Defecto: 1", simSeed);

  cmd.Parse (argc, argv);
  
  NS_LOG_UNCOND("Ns3Env parameters:");
  NS_LOG_UNCOND("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND("--openGymPort: " << openGymPort);
  NS_LOG_UNCOND("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND("--seed: " << simSeed);
  NS_LOG_UNCOND("--testArg: " << testArg);

    RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);


   Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openGym->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openGym->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openGym->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  openGym->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openGym->SetGetExtraInfoCb( MakeCallback (&MyGetExtraInfo) );
  openGym->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );
  Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);


  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Ratio de envio de información en capa de aplicación
  std::string appRate ("1024bps");
  

  /* Configuración de generador de aplicación generadora de tráfico según patron 
   * de on/off, y un ratio de bits constante
   */
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue (packetSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (appRate));

  // El modelo wifi es capaz de enviar información desde un nodo a varios
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  // Creación de nodos, todos en un mismo contenedor
  NodeContainer manetNodes;
  manetNodes.Create (N_NODES);

  WifiHelper wifi;

  // Con el modo verbose activo, se activan los registros de wifi
  if (verbose)
    {
      wifi.EnableLogComponents (); 
    }

  // Uso de estandar 802.11b en modelo wifi
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  // Configuración por defecto del modelo físico de wifi
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  /* Uso del modelo de perdidas con poder de señales constante, 
   * independientemente de la distancia entre los nodos
   */
  //wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (txp));

  // Todos los objetos de capa física comparten el mismo canal 
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Configuración de capa MAC
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
  // Establecer modo ad hoc
  wifiMac.SetType ("ns3::AdhocWifiMac");

  /* Se instala el modelo con las capas física y mac establecidas,
   * en nuestros nodos creados, obteniendo los dispositivos de red
   * que interactuan en la manet
   */
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, manetNodes);


  MobilityHelper mobility;

  /* Configuración de espacio de movilidad que tienen los nodos, después
   * de cada pausa según el modelo Random Waypoint 
   */  
  ObjectFactory mobilityWayPoint;
  mobilityWayPoint.SetTypeId("ns3::RandomBoxPositionAllocator");
  mobilityWayPoint.Set("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=2000]"));
  mobilityWayPoint.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=2000]"));
  mobilityWayPoint.Set("Z", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));

  Ptr<PositionAllocator> wayPointAllocator = mobilityWayPoint.Create()->GetObject<PositionAllocator>();

  /* Definición de espacio de instanciamiento de cada nodo dentro de una
   * caja 3d de 500m x 500m x 30m
   */  
  mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                  "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"),
                                  "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"),
                                  "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));

  std::stringstream pauseVariable;
  pauseVariable << "ns3::ConstantRandomVariable[Constant=" << nodePauseTime << "]";

  // Uso de modelo de movilidad Random Waypoint
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                   "Speed", StringValue("ns3::UniformRandomVariable[Min=5|Max=10]"),
                                   "Pause", StringValue(pauseVariable.str ()),
                                   "PositionAllocator", PointerValue(wayPointAllocator));
  mobility.Install (manetNodes);

  // Helpers de protocolos de enrutamiento para una red manet
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;

  /* Helper de lista de protocolos que definen funciones de
   * enrutamiento y envio de paquetes
   */
  Ipv4ListRoutingHelper list;

  // Stack de protocolos de internet estándar
  InternetStackHelper internet;

  switch (protocolType)
    {
    case 1:

      /* Añade protocolo proactivo OLSR a la lista de protocolos
       * con una prioridad de 100
       */

      list.Add (olsr, 100);
      protocolTypeName = "OLSR";
      break;
    case 2:

      /* Añade protocolo reactivo AODV a la lista de protocolos
       * con una prioridad de 100
       */
      list.Add (aodv, 100);
      protocolTypeName = "AODV";
      break;
    case 3:

      /* Añade protocolo proactivo DSDV a la lista de protocolos
       * con una prioridad de 100
       */
      list.Add (dsdv, 100);
      protocolTypeName = "DSDV";
      break;
    }

  if (protocolType < 4)
    {
      // Establecer protocolos de enrutamiento en la pila tcp/ip
      internet.SetRoutingHelper (list);
      
      // Instalacion de pila tcp/ip en nodos
      internet.Install (manetNodes);
    }
  else if (protocolType == 4)
    {
      protocolTypeName = "DSR";
      // Instalacion de pila tcp/ip en nodos
      internet.Install (manetNodes);

      // Establecer protocolo de enrutamiento DSR en la red manet
      dsrMain.Install (dsr, manetNodes);
    } 
  
  //Se imprimen las posiciones iniciales de los nodos
  NS_LOG_UNCOND ("El protocolo de enrutamiento usado es " << protocolTypeName);

  //Se imprimen las posiciones iniciales de los nodos
  NS_LOG_UNCOND ("\nUbicación inicial de cada nodo en un espacio 3D(x:y:z)\n");
  for (uint32_t m = 0; m < manetNodes.GetN(); m++)
  {
    Ptr<Node> node = manetNodes.Get(m);
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
    NS_LOG_UNCOND ("Nodo " << node->GetId() << " -> " << mobility->GetPosition());
  }

  NS_LOG_UNCOND ("\n");
  //Establecer direcciones ip en cada nodo de red manet
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
    
  // Creación de aplicación on/off, se usa 1 segundo de prendido, 0.5 segundos de apagado
  OnOffHelper onoffapp ("ns3::UdpSocketFactory",Address ());
  onoffapp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoffapp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
    
  // Se itera sobre los sumideros y se instala la aplicación en otros nodos
  for (int s = 0; s < 10; s++)
    {
      //Creación de socket tipo UDP
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      Ptr<Socket> sink = Socket::CreateSocket (manetNodes.Get(s), tid);
      InetSocketAddress local = InetSocketAddress(i.GetAddress (s), 9);

      sink->Bind (local);
      sink->SetRecvCallback (MakeCallback (&ReceivePacket));

      AddressValue remoteAddress (InetSocketAddress ( i.GetAddress(s), 9));

      onoffapp.SetAttribute("Remote", remoteAddress);

      ApplicationContainer temp = onoffapp.Install(manetNodes.Get(10 + s));
      temp.Start(Seconds(2 + s));
      temp.Stop (Seconds(stopTime));
      
      // Establecemos tracing en cada uno de los sumideros
      wifiPhy.EnablePcap ("manetNetworkTrace", devices.Get (s));
    }
    
    // Instalación de aplicacion UdpEcho
    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (manetNodes.Get (20));
    wifiPhy.EnablePcap ("manetNetworkTrace", devices.Get (20));
    serverApps.Start (Seconds (1));
    serverApps.Stop (Seconds (stopTime));
    
    // Se establece el logging que viene por defecto con esta aplicación
    // tanto de lado de cliente como de servidor
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    for (int s = 0; s < 5; ++s) 
      {
        // Se itera sobre cada nodo para enviar paquetes al servidor
        UdpEchoClientHelper echoClient (i.GetAddress (20), 9);
        echoClient.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
        echoClient.SetAttribute ("Interval", TimeValue (Seconds (interval)));
        echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
        
        // Se instala aplicación 
        ApplicationContainer clientApps = echoClient.Install (manetNodes.Get (21+s));
        clientApps.Start (Seconds (3+s));
        clientApps.Stop (Seconds (stopTime));
      }

    
  // Se hace uso del FlowMonitor para rescatar información útil y generar análisis y estadísticas
  Ptr<FlowMonitor> monitor;
  FlowMonitorHelper monitorHelper;
  monitor = monitorHelper.InstallAll ();


  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  
  monitor->SerializeToXmlFile ("manetNetworkMonitor.xml", false, false);
  NS_LOG_UNCOND ("\nNúmero total de bytes enviados: " << NBYTESTOTAL << "B");
  NS_LOG_UNCOND ("Número total de paquetes recibidos: " << NRECEIVEDPACKETS << "\n");

  Simulator::Destroy ();
  return 0;
}
