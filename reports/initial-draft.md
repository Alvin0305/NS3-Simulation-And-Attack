# Industrial IoT Security Simulation Using CoAP in NS-3

## 1. Introduction

Industrial Internet of Things (IIoT) systems connect sensors, programmable logic controllers (PLCs), and actuators to monitor and control industrial processes. Communication between these components is essential for reliable operation, while unauthorized control commands can compromise the safety and availability of the system.

This project develops an educational IIoT testbed using the NS-3 network simulator. The Constrained Application Protocol (CoAP) is used for communication between the simulated devices. A temperature-control scenario is used to demonstrate normal operation and investigate the security implications of accepting unauthorized actuator commands.

The project will evaluate an attack scenario and a corresponding security mitigation.

## 2. Objectives

The objectives are:

1. Simulate an IIoT network containing a sensor, PLC, actuator, and attacker.
2. Implement CoAP-based communication between the industrial components.
3. Simulate temperature monitoring and threshold-based control.
4. Investigate unauthorized command injection against an actuator.
5. Implement and evaluate a mitigation that restricts unauthorized control commands.
6. Compare normal operation, attack behavior, and defended operation using simulation logs and packet captures.

## 3. Simulation Environment

The system is implemented in C++ using NS-3. The network consists of four nodes connected through a CSMA network.

| Component | IPv4 address | Function |
|---|---|---|
| Temperature sensor | 10.1.1.1 | Sends temperature readings |
| PLC controller | 10.1.1.2 | Processes readings and determines the cooler state |
| Cooler actuator | 10.1.1.3 | Processes cooler-control commands |
| Attacker | 10.1.1.4 | Sends an unauthorized control command |

The network uses the subnet 10.1.1.0/24. CoAP messages are transported over UDP, using port 5683.

The current implementation contains a minimal CoAP encoder and decoder supporting the message types, codes, tokens, URI-Path option, and payloads required by the experiment. It is an educational subset rather than a complete implementation of the CoAP specification.

## 4. System Design

### 4.1 Temperature monitoring

The sensor sends temperature readings to the PLC using CoAP POST requests to the `/temperature` resource.

The PLC uses a threshold of 70°C to determine the required cooler state.

- If the temperature is greater than 70°C, the PLC commands the cooler to turn ON.
- If the temperature is less than or equal to 70°C, the PLC commands the cooler to turn OFF.

### 4.2 Actuator control

The PLC sends CoAP POST requests to the actuator's `/command` resource. The payload specifies either `COOLER ON` or `COOLER OFF`.

The actuator responds with CoAP code 2.04 Changed (decimal 68) when it accepts a recognized command.

### 4.3 Communication protocol

The implementation constructs CoAP messages in C++ and sends them through NS-3 UDP sockets. The messages include the CoAP header, message ID, token, URI-Path option, and payload where applicable.

Packet captures will be examined to verify the encoded messages and their exchanges over the simulated network.

## 5. Baseline Experiment

The first experiment evaluates normal temperature monitoring and control without an attacker command.

The sensor transmits four readings.

| Simulation time | Temperature | PLC decision | Expected cooler state |
|---|---:|---|---|
| 1 s | 65°C | COOLER OFF | OFF |
| 2 s | 85°C | COOLER ON | ON |
| 3 s | 85°C | COOLER ON | ON |
| 4 s | 55°C | COOLER OFF | OFF |

The simulation logs confirm that the PLC receives the temperature requests, evaluates the threshold, and sends the corresponding commands. The actuator acknowledges the commands, and the sensor receives successful responses from the PLC.

The baseline establishes that the intended application-level control flow is operational.

## 6. Unauthorized Command Injection Experiment

### 6.1 Threat model

The attacker is represented by a node connected to the same simulated LAN as the sensor, PLC, and actuator. The attacker can transmit UDP packets containing syntactically valid CoAP messages.

The current actuator checks the CoAP request type, method, resource path, and command payload. However, it does not authenticate the sender or restrict control commands to the PLC.

The experiment investigates whether a node other than the PLC can issue a control command that the actuator accepts.

### 6.2 Attack procedure

At simulation time 2.5 seconds, the attacker sends a CoAP POST request to the actuator's `/command` resource with the payload `COOLER OFF`.

This occurs after the sensor has reported 85°C and the PLC has issued a `COOLER ON` command.

### 6.3 Observed results

The attacker log contains the following events:

- At 2.500 s, the attacker starts sending the unauthorized command.
- It sends `POST /command` with payload `COOLER OFF` and message ID 500.
- At 2.504 s, it receives response code 68, corresponding to CoAP 2.04 Changed.

The successful response indicates that the actuator accepted the attacker's request under the current implementation.

This demonstrates an authorization weakness: the actuator does not distinguish a command from the legitimate PLC from a command sent by the attacker.

A persistent actuator-state variable and explicit state logging are still required to verify the resulting state change independently.

## 7. Current Limitations

The current implementation has several limitations:

1. The CoAP implementation supports only a subset of the protocol.
2. The actuator does not authenticate or authorize command senders.
3. The actuator does not yet maintain a persistent cooler-state variable.
4. The current experiment does not reproduce the full complexity of the Stuxnet attack.
5. The simulation does not yet include a completed defense or a quantitative comparison of attack and defended behavior.

These limitations will be addressed or discussed in the final report.

## 8. Planned Work

The next stages are:

1. Add persistent actuator-state tracking and explicit state-change logs.
2. Validate the CoAP packet format using the generated PCAP files.
3. Implement a defense against unauthorized actuator commands.
4. Repeat the attack with the defense enabled.
5. Record accepted and rejected commands and compare the outcomes.
6. Document the threat model, experimental results, and limitations.
7. Prepare a demonstration video and final submission.
