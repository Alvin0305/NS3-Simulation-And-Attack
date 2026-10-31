#include "Cooler.h"
#include "PLC.h"
#include "Sensor.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
  NodeContainer nodes;
  nodes.Create(4);

  Ptr<Node> sensor = nodes.Get(0);
  Ptr<Node> plc = nodes.Get(1);
  Ptr<Node> cooler = nodes.Get(2);
  Ptr<Node> attacker = nodes.Get(3);

  InternetStackHelper internet;
  internet.Install(nodes);

  CsmaHelper csma;

  csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

  NetDeviceContainer devices = csma.Install(nodes);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign(devices);

  const Ipv4Address sensorAddress = interfaces.GetAddress(0);
  const Ipv4Address plcAddress = interfaces.GetAddress(1);
  const Ipv4Address coolerAddress = interfaces.GetAddress(2);
  const Ipv4Address attackerAddress = interfaces.GetAddress(3);

  std::cout << "\n========== IIoT LAN ==========\n";
  std::cout << "Sensor:   " << sensorAddress << "\n";
  std::cout << "PLC:      " << plcAddress << "\n";
  std::cout << "Actuator: " << coolerAddress << "\n";
  std::cout << "Attacker: " << attackerAddress << "\n";
  std::cout << "==============================\n\n";

  Ptr<Sensor> sensorApp = CreateObject<Sensor>();
  sensorApp->SetDestination(plcAddress, 4000);
  sensorApp->SetInterval(Seconds(1.0));
  sensor->AddApplication(sensorApp);
  sensorApp->SetStartTime(Seconds(1.0));
  sensorApp->SetStopTime(Seconds(5.0));

  Ptr<PLC> plcApp = CreateObject<PLC>();
  plcApp->SetActuator(coolerAddress, 5000);
  plcApp->SetThreshold(70.0);
  plc->AddApplication(plcApp);
  plcApp->SetStartTime(Seconds(0.0));
  plcApp->SetStopTime(Seconds(5.5));

  Ptr<Cooler> coolerApp = CreateObject<Cooler>();
  coolerApp->setPort(5000);
  cooler->AddApplication(coolerApp);
  coolerApp->SetStartTime(Seconds(0.0));
  coolerApp->SetStopTime(Seconds(5.5));

  csma.EnablePcapAll("iiot-lan", true);

  std::cout << "\nStarting IIoT Temperature Simluation\n" << std::endl;

  Simulator::Stop(Seconds(6.0));

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "\nSimulation completed." << std::endl;
  return 0;
}
