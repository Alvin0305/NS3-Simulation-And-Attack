#include "Cooler.h"
#include "PLC.h"
#include "Sensor.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
  NodeContainer nodes;
  nodes.Create(3);

  Ptr<Node> sensor = nodes.Get(0);
  Ptr<Node> plc = nodes.Get(1);
  Ptr<Node> cooler = nodes.Get(2);

  InternetStackHelper internet;
  internet.Install(nodes);

  PointToPointHelper p2p;

  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  NetDeviceContainer sensorPlcDevices = p2p.Install(sensor, plc);
  NetDeviceContainer plcActuatorDevices = p2p.Install(plc, cooler);

  Ipv4AddressHelper address;

  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer plcToSensorInterface =
      address.Assign(sensorPlcDevices);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer plcToCoolerInterface =
      address.Assign(plcActuatorDevices);

  const Ipv4Address plcToSensorAddress = plcToSensorInterface.GetAddress(1);
  const Ipv4Address plcToCoolerAddress = plcToCoolerInterface.GetAddress(1);

  Ptr<Sensor> sensorApp = CreateObject<Sensor>();
  sensorApp->SetDestination(plcToSensorAddress, 4000);
  sensorApp->SetInterval(Seconds(1.0));
  sensor->AddApplication(sensorApp);
  sensorApp->SetStartTime(Seconds(1.0));
  sensorApp->SetStopTime(Seconds(5.0));

  Ptr<PLC> plcApp = CreateObject<PLC>();
  plcApp->SetActuator(plcToCoolerAddress, 5000);
  plcApp->SetThreshold(70.0);
  plc->AddApplication(plcApp);
  plcApp->SetStartTime(Seconds(1.0));
  plcApp->SetStopTime(Seconds(5.5));

  Ptr<Cooler> coolerApp = CreateObject<Cooler>();
  coolerApp->setPort(5000);
  cooler->AddApplication(coolerApp);
  coolerApp->SetStartTime(Seconds(0.0));
  coolerApp->SetStopTime(Seconds(5.5));

  p2p.EnablePcapAll("iiot-basic", true);

  std::cout << "\nStarting IIoT Temperature Simluation\n" << std::endl;

  Simulator::Stop(Seconds(6.0));

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "\nSimulation completed." << std::endl;
  return 0;
}
