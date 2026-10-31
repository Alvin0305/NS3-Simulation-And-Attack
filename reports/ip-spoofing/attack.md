# IP Spoofing and Authentication in an Industrial IoT Testbed

## 1. Objective

The objective of this experiment is to investigate the security of Machine-to-Machine (M2M) communication in an Industrial Internet of Things (IIoT) environment. The experiment uses the Constrained Application Protocol (CoAP) over UDP to simulate communication between a temperature sensor, a Programmable Logic Controller (PLC), and a cooler actuator.

The experiment evaluates whether an IP-address-based access-control mechanism can prevent an unauthorized node from issuing commands to an industrial actuator. It also establishes a baseline for evaluating stronger authentication mechanisms.

## 2. Simulation Environment

The testbed is implemented using NS-3.49 and consists of four nodes connected through a shared CSMA network.

| Component | IP address | Function |
|---|---|---|
| Temperature Sensor | 10.1.1.1 | Sends temperature readings to the PLC |
| PLC Controller | 10.1.1.2 | Evaluates temperature readings and controls the cooler |
| Cooler Actuator | 10.1.1.3 | Receives and executes ON/OFF commands |
| Attacker | 10.1.1.4 | Generates unauthorized control requests |

**Network configuration**
- Network: 10.1.1.0/24
- Communication protocol: CoAP over UDP
- CoAP server port: 5683
- Temperature threshold: 70°C

The simulation uses a simplified educational implementation of CoAP rather than a complete implementation of the protocol specification.

## 3. Normal System Operation

The temperature sensor periodically sends readings to the PLC. The PLC compares each reading against the configured threshold and sends a corresponding command to the cooler.

The control logic is:

- Temperature below the threshold: turn the cooler OFF.
- Temperature above the threshold: turn the cooler ON.

The simulated temperature readings are 65°C, 85°C, 85°C, and 55°C.

The resulting legitimate control sequence is:

| Temperature | PLC decision | Expected cooler state |
|---:|---|---|
| 65°C | COOLER OFF | OFF |
| 85°C | COOLER ON | ON |
| 85°C | COOLER ON | ON |
| 55°C | COOLER OFF | OFF |

The PLC and cooler communicate using CoAP POST requests. Successful requests receive a CoAP 2.04 Changed response in this implementation.

## 4. Access-Control Mechanism

An IP-address allowlist is configured at the cooler. The cooler accepts control commands only when the sender's observed IPv4 source address matches the PLC address, 10.1.1.2.

Requests from other source addresses are rejected with CoAP response code 131, corresponding to 4.03 Forbidden.

This mechanism prevents ordinary unauthorized requests from nodes that use their own IP addresses. However, it does not cryptographically authenticate the sender.

## 5. Attack Experiment

The attacker node attempts to send a CoAP POST request to the cooler with the payload `COOLER OFF`.

Two scenarios are considered:

### Scenario A: Unauthorized request without source-address spoofing

The attacker sends the request using its own IP address, 10.1.1.4.

The cooler observes that the request originates from an unauthorized address and rejects it.

The corresponding log entry is:

`ACCESS DENIED. source=10.1.1.4, authorized plc=10.1.1.2`

The cooler returns CoAP response code 131. This confirms that the IP-address allowlist blocks the ordinary unauthorized request.

### Scenario B: Request with a forged source address

In the second scenario, the experiment constructs a request whose IPv4 source address is set to the PLC's address, 10.1.1.2.

The cooler receives the request with message ID 500 and observes the source address as 10.1.1.2. Because the current access-control mechanism relies on this address, the request passes the allowlist check.

The cooler's log records:

`Received CoAP request from=10.1.1.2, path=/command, MID=500, payload=COOLER OFF`

It subsequently records:

`Status changed: ON -> OFF, source=10.1.1.2`

These observations establish that the forged-source request was accepted and that the actuator state changed as a result.

## 6. Results and Analysis

The experiment demonstrates two different outcomes.

| Test | Source address observed by cooler | Result |
|---|---|---|
| Ordinary unauthorized request | 10.1.1.4 | Rejected |
| Forged-source request | 10.1.1.2 | Accepted; cooler state changed |

The IP allowlist works when the sender uses its actual source address. However, the source IP address alone is insufficient to establish the identity of a sender because it is a network-layer addressing field, not cryptographic proof of identity.

An attacker who can inject a packet with a forged source address may therefore bypass this particular authorization check in the simulated environment.

The experiment demonstrates a limitation of the implemented access-control mechanism. It does not establish that every real industrial network is vulnerable in the same way; actual exploitability depends on network architecture, filtering, routing, and other security controls.

## 7. Proposed Countermeasures

The primary countermeasure is to replace IP-address-only authorization with cryptographic message authentication.

The PLC and cooler should share a secret key that is unavailable to the attacker. Every control command should carry a Message Authentication Code (MAC), computed over the command's authenticated fields.

The cooler should verify the MAC before executing the command. A request with an invalid or missing MAC should be rejected, regardless of its source IP address.

The authenticated data should include:
- The identity of the authorized controller.
- The command resource or destination.
- The command payload.
- A monotonically increasing sequence number or another suitable freshness value.
- Any protocol context necessary to prevent the MAC from being reused for a different purpose.

Replay protection should ensure that a previously accepted command cannot simply be transmitted again and treated as a new authenticated command.

HMAC-SHA-256 is one suitable construction for this educational implementation. The secret key must be provisioned securely and must not be available to the attacker node.

For a production system, established security mechanisms such as Datagram Transport Layer Security (DTLS) or Object Security for Constrained RESTful Environments (OSCORE) should also be evaluated, alongside network segmentation and monitoring.

## 8. Conclusion

The NS-3 experiment demonstrates that IP-address-based access control can reject ordinary unauthorized requests but may fail when the source address is forged.

The cooler accepted the forged-source request because the observed source address matched the configured PLC address. This highlights the difference between identifying a network address and authenticating the sender.

The next phase of the experiment will implement cryptographic command authentication and replay protection. The same attack scenarios will then be repeated to evaluate whether the cooler rejects forged-source requests that lack a valid authentication code.

This approach provides a controlled way to demonstrate both the weakness of IP-only authorization and the importance of message-level authentication in IIoT control systems.
