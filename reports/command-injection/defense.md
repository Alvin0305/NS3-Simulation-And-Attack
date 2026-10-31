## Defense Experiment: Source-Based Access Control

### Objective

The objective of this experiment is to determine whether restricting actuator commands to an authorized PLC prevents unauthorized command injection.

### Defense mechanism

The actuator implements a source-IP allowlist. Commands received from the configured PLC address, 10.1.1.2, are eligible for processing. Commands from other source addresses are rejected before the command payload can change the simulated cooler state.

Accepted commands receive a CoAP 2.04 Changed response. Valid control requests from unauthorized senders receive a CoAP 4.03 Forbidden response.

### Experimental procedure

The temperature sensor reports 85°C, causing the PLC to issue a `COOLER ON` command. The attacker subsequently sends the same unauthorized `COOLER OFF` command used in the unprotected experiment.

The experiment records the actuator's decisions, the resulting cooler state, and the response received by the attacker.

### Expected outcome

The actuator accepts the PLC's command and maintains the cooler in the ON state. It rejects the attacker's command and returns 4.03 Forbidden. The cooler should remain ON after the rejected request.

### Limitations

Source-IP filtering is a basic access-control mechanism. It does not provide cryptographic authentication and may be insufficient against an attacker capable of spoofing an authorized source address. A stronger implementation would authenticate control messages and incorporate replay protection.
