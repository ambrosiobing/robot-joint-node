| # | Chapter | Theme |
|---|---|---|
| 1 | [What a joint node is, and the bench that stands in for one](chapters/01-what-a-joint-node-is.md) | Architecture, the stand-in bench, what is real and what is modelled |
| 2 | [The control period: 1 kHz you can prove](chapters/02-the-control-period.md) | Timer-driven period, jitter, measuring it without a scope |
| 3 | [One clock for sensors, loop and bus](chapters/03-one-clock-for-sensors.md) | Timestamping, a monotonic tick, what to do without precision time hardware |
| 4 | [The board support package, and a board file you can hand over](chapters/04-the-board-support-package.md) | Pin map, clock config, peripheral table, a board file another engineer can read |
| 5 | [The encoder: quadrature in hardware, and one you generate](chapters/05-the-encoder.md) | Timer encoder mode, index, velocity from differences, generating quadrature to test the decoder |
| 6 | [The inertial unit as the joint's inner ear](chapters/06-the-inertial-unit-as-the-joints-inner-ear.md) | Rate and acceleration, FIFO, watermark, timestamp alignment with the control period |
| 7 | [The actuator you do not have: PWM, dead time, and a plant model](chapters/07-the-actuator-you-do-not-have.md) | Complementary PWM, dead time, fault input, a second-order plant in software |
| 8 | [Force and torque: the signal you cannot buy](chapters/08-force-and-torque.md) | What the signal is, what it is for, what a real sensor needs, and the error of the stand-in |
| 9 | [CAN-FD from the controller out: bit timing and the first frame](chapters/09-can-fd-from-the-controller-out.md) | Bit timing for both phases, the transceiver, sample point, the first frame on a wire |
| 10 | [The motion master: the bus on Linux](chapters/10-the-motion-master.md) | Socket layer on the host, the command line tools, a Python setpoint source |
| 11 | [A joint protocol: state and command in sixty-four bytes](chapters/11-a-joint-protocol.md) | Frame layout, identifier allocation and priority, what belongs in a state frame |
| 12 | [Network management: heartbeat, node state, bus-off and recovery](chapters/12-network-management.md) | Heartbeat, node state machine, error counters, bus-off detection and recovery |
| 13 | [Two speeds on one wire, and why the old node errors](chapters/13-two-speeds-on-one-wire.md) | Mixed classic and flexible-data traffic, tolerance, what a non-tolerant controller does |
| 14 | [The node as a middleware participant, and the agent that hosts it](chapters/14-the-node-as-a-middleware-participant.md) | The embedded middleware client, the agent on the host, transport and footprint |
| 15 | [Joint state and joint command as messages](chapters/15-joint-state-and-joint-command-as-messages.md) | The standard message types, units and conventions, mapping frames to messages |
| 16 | [The loop closed over the bus: setpoint in, state out, following error](chapters/16-the-loop-closed-over-the-bus.md) | Setpoint injection, the controller, following error as the figure of merit |
| 17 | [What a real-time fieldbus would change, and why it is not on this bench](chapters/17-what-a-real-time-fieldbus-would-change.md) | Slave hardware, cycle time, the open master stack, what would have to be bought |
| 18 | [Safe states, and the workspace sensor that triggers one](chapters/18-safe-states.md) | Safe-state machine, latching, entry actions, a presence sensor as a trigger |
| 19 | [Update over the bus: a node you can reach but not touch](chapters/19-update-over-the-bus.md) | Transport for update, a bootloader that stays addressable, versioning across a fleet |
| 20 | [The rig: injected faults, tracking error, and a build that fails](chapters/20-the-rig.md) | Fault injection, tracking regression, a report that turns a build red |
