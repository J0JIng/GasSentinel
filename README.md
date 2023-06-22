# ~OdorGuard~

## Overview 
Gas leaks can be dangerous but hard to detect. This project aims to use edge AI to provide low-latency, accurate detection of gas leaks. In particular, these are propane, butane and natural gas.<br>

## Algorithm and Detection Principles
Interpreting gas sensor resistances is complex and may require specialized knowledge in electrochemical sensor behaviors. Resistance of the sensing element is nontrivially influenced by ambient conditions, and even past gas exposure events. As such, this situation provides an opportunity to utilize ML techniques to interpret the data.<br>

This project utilizes the BSEC software (which runs on the microcontroller) to run the associated signal processing/fusion and ML model. Refer to Figure 1 below for the confusion matrix and F1 scores:<br>

img<br>
Figure 1: Confusion matrix and F1 scores<br>

The model was trained with low concentrations of butane, propane, etc. at room temperature/pressure at (or above) their respective Lower Explosive Limit (LEL). The model runs every 73.36s with a duty cycle of 25%.<br>
Additionally, for each $t \in [0, 18.34]s$ during a measurement cycle, the sensor heater is configured to have the profile[^1]:
```math
h(t) = \begin{cases}
  100°C, & \text{if } 0.28s \leq t \leq 6.02s, \\
  200°C, & \text{if } 6.3s \leq t \leq 12.18s, \\
  320°C, & \text{if } 12.46s \leq t \leq 18.34s.
\end{cases}
```
where the undefined intervals are transition (i.e. ramp-up/down) periods. This heater activity is responsible of the majority of the power consumption (>12mA), where other loads (i.e. microcontroller+RF) are negligible. Hence, the battery life is almost directly proportional to the measurement duty cycle. For this particular application, a duty cycle of 25% was found to yield a good balance between response time (73.36s) and power consumption (avg. 2.1mA).
<br><br>

## Design
For this particular application, it is vital that warning latencies are kept low. This project provides on-device inference, which allows for rapid reaction to events, and low data transmission overhead.<br>
Additionally, the device has a low-latency Thread network connection (compared to BLE/Zigbee) for offsite notifications and action triggering, which is also low-power, allowing for operation either from USB power, or from AA batteries.<br>
The algorithms described above are run on a EFR32MG24 series microcontroller, with integrated 802.15.4 radio for Thread communications.<br>
The application layer overview for the microcontroller software can be found in Figure 2:<br>
![SW](https://github.com/J0JIng/OdorGuard/tree/main/Code/sw.png)<br>
Figure 2: Application-Layer Block Diagram (Microcontroller)<br>



[^1]: This profile was found to yield the highest F1 score and generalization to test/live data.