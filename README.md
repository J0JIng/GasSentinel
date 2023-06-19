# OdorGuard

## Overview 
Gas leaks can be dangerous but hard to detect. This project aims to use edge AI to provide low-latency, accurate detection of gas leaks. In particular, these are propane, butane and natural gas.<br>

## Design
Interpreting gas sensor resistances is complex and may require specialized knowledge in electrochemical sensor behaviors. Resistance of the sensing element is nontrivially influenced by ambient conditions, and even past gas exposure events. As such, this situation provides an opportunity to utilize ML techniques to interpret the data.<br>
For this particular application, it is vital that warning latencies are kept low. This project provides on-device inference, which allows for rapid reaction to events, and low data transmission overhead.<br>
Additionally, the device has a low-latency Thread network connection (compared to BLE/Zigbee) for offsite notifications and action triggering, which is also low-power, allowing for operation either from USB power, or from AA batteries.