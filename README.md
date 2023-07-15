# GasSentinel

## Overview 
Gas leaks can be dangerous but hard to detect. This project aims to use edge AI to provide low-latency, accurate detection of gas leaks. In particular, these are propane, butane and natural gas.<br>
It is also possible to detect indicative gases such as H<sub>2</sub>S.

## Algorithm and Detection Principles
Interpreting gas sensor resistances is complex and may require specialized knowledge in electrochemical sensor behaviors. Resistance of the sensing element is nontrivially influenced by ambient conditions, and even past gas exposure events. As such, this situation provides an opportunity to utilize ML techniques to interpret the data.<br>

This project utilizes the BSEC software (which runs on the microcontroller) to run the associated signal processing/fusion and ML model. Refer to Figure 1 below for the confusion matrix and F1 scores:<br>

![Confusion Matrix and F1](https://github.com/J0JIng/OdorGuard/blob/main/Doc/f1_lp.png)<br>
Figure 1: Confusion matrix and F1 scores (LP)<br>

The model was trained with low concentrations of butane, propane, etc.[^1] at room temperature/pressure at (or above) their respective Lower Explosive Limit (LEL). The model runs every 73.36s with a duty cycle of 25%.<br>
During training, the sensor was exposed to the following mixtures/gases:
| Mixture/Gas          | Class    |
|----------------------|----------|
| Outdoor Air          | Baseline |
| Indoor Air (Normal)  | Baseline |
| Indoor Air (A/C)     | Baseline |
| Indoor Air (Cooking) | Baseline |
| Perfumes             | Baseline |
| Expired Air          | Baseline |
| LPG Mix              | Gas Leak |
| Butane               | Gas Leak |

<br>


Additionally, for each $t \in [0, 18.34]s$ during a measurement cycle, the sensor heater is configured to have the profile[^2]:
```math
h(t) = \begin{cases}
   100°C, & \text{if } 0.28s \leq t \leq 6.02s, \\
  200°C, & \text{if } 6.3s \leq t \leq 12.18s, \\
  320°C, & \text{if } 12.46s \leq t \leq 18.34s.
\end{cases}
```
where the undefined intervals are transition (i.e. ramp-up/down) periods. This heater activity is responsible for the majority of the power consumption (>12mA), where other loads (i.e. microcontroller+RF) are negligible. Hence, the battery life is almost directly proportional to the measurement duty cycle. For this particular application, a duty cycle of 25% was found to yield a good balance between response time (73.36s) and power consumption (avg. 2.1mA).<br>
Note that if USB power is detected at startup, a 100% duty cycle high performance model will be loaded instead.
<br>

## System Design
For this particular application, it is vital that warning latencies are kept low. This project provides on-device inference, which allows for rapid reaction to events, and low data transmission overhead.<br>
Additionally, the device has a low-latency Thread network connection (compared to BLE/Zigbee) for offsite notifications and action triggering, which is also low-power, allowing for operation either from USB power, or from AA batteries. <br>

The power consumed by the stepwise heater function accounts for the majority of the system power consumption. The system quiescent current I<sub>Q</sub> by design is stated below:<br>
$I_{Q} = I_{EM2(MGM240P)} + I_{DDSB(BME688)} + I_{Q(TPS2116)} + I_{SD(MX95)} + I_{SW(ST1PS03)}$<br>
$I_{Q} = 2.9μA + 0.15μA + 1.35μA + 7nA + 1μA$<br>
$I_{Q} = 5.4μA$<br>
Which is reflected in measurements. <br>

<br>
<div>
    <img src="https://github.com/J0JIng/OdorGuard/blob/main/Doc/pcb.jpg" alt="PCB" width="400" />
    <img src="https://github.com/J0JIng/OdorGuard/blob/main/Doc/case.jpg" alt="Case" width="400" />
</div>

Figure 2: GasSentinel Custom Board and Enclosure<br>

The algorithms described above are run the onboard EFR32MG24 series microcontroller, with integrated 802.15.4 radio for Thread communications.<br>
The application layer overview for the microcontroller software can be found in Figure 3:<br><br>
![SW](https://github.com/J0JIng/OdorGuard/blob/main/Doc/sw.png)<br>
Figure 3: Application-Layer Block Diagram (Microcontroller)<br>
<br>
The device will also send the inference results and other metadata over CoAP to InfluxDB for backend integration and data analytics/visualization.<br>

![Seq](https://github.com/J0JIng/OdorGuard/blob/main/Doc/seq.jpg)<br>
Figure 4: Messaging Sequence Diagram

## Performance Evaluation
The qualitative results of the low-power classifier are shown in Figure 5, over a period of approx. 2h, the device was subjected to 3 periods of the positive class (Gas Leak), denoted by the "Ground Truth" label.<br>
In general, the model has a recovery period of minimally a few cycles, after being exposed to the positive class. Some of this behavior can be attributed to the behaviors of the MOx element. In particular, the first instance of the positive class in Figure 5 shows that the recovery time is much greater after exposure to high concentrations of gas.<br>

It is notable that in all instances, (correct) detection occured within one cycle (i.e. 73.36s), satisfying the objective of rapid classification.<br>
![Eval](https://github.com/J0JIng/OdorGuard/blob/main/Doc/eval.jpg)<br>
Figure 5: Qualitative results of the low-power classifier
## 
[^1]: Note that methane (despite being found in gas leaks) was categorically avoided, as methane cannot be detected significantly by the sensor.
[^2]: This profile was found to yield the highest F1 score and generalization to test/live data.
