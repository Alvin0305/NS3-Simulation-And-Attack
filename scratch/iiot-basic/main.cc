#include "attacker/Attacker.h"
#include "cooler/Cooler.h"
#include "plc/PLC.h"
#include "sensor/Sensor.h"
#include "utils/ConfigParser.h"

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

using namespace ns3;

int main(int argc, char* argv[]) {
    ConfigParser parser;
    IIoTConfig config = parser.parse("config.no-hmac-defense.json");

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
    sensorApp->SetDestination(plcAddress);
    sensor->AddApplication(sensorApp);
    sensorApp->SetStartTime(Seconds(1.0));
    sensorApp->SetStopTime(Seconds(5.0));

    Ptr<PLC> plcApp = CreateObject<PLC>();
    plcApp->SetCoolerAddress(coolerAddress);
    plcApp->SetThreshold(70.0);
    plc->AddApplication(plcApp);
    plcApp->SetStartTime(Seconds(0.0));
    plcApp->SetStopTime(Seconds(5.5));

    Ptr<Cooler> coolerApp = CreateObject<Cooler>(config);
    cooler->AddApplication(coolerApp);
    coolerApp->SetStartTime(Seconds(0.0));
    coolerApp->SetStopTime(Seconds(5.5));

    Ptr<Attacker> attackerApp = CreateObject<Attacker>(config);
    attackerApp->SetTarget(coolerAddress);
    attacker->AddApplication(attackerApp);
    attackerApp->SetStartTime(Seconds(0.0));
    attackerApp->SetStopTime(Seconds(5.0));

    csma.EnablePcapAll("iiot-coap", true);

    std::cout << "\nStarting IIoT CoAP Simluation\n" << std::endl;

    Simulator::Stop(Seconds(6.0));

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "\nSimulation completed." << std::endl;
    return 0;
}
