# XGE-PTPv2

### Introduction
Hardware engine design for PTPv2 protocol in 10G Ethernet, described in Verilog HDL<br>

### Design Features

1. Built-in high-precision RTC supporting PPS output and input
2. Fully supports both One-step Clock and Two-step Clock operation modes as described in IEEE Std 1588-2008/2019
3. Supports both Delay Request-Response and Peer Delay mechanisms
4. Supports IEEE1588v2 packets in various encapsulation formats:
   - PTP over IEEE802.3/Ethernet (supports 802.1AS)
   - PTP over UDP IPv4/IPv6
   - No VLAN/Single VLAN/Double VLAN
5. Full hardware implementation of Transparent Clock (TC Hardware Offload)
6. Precise and efficient timestamp processing:
   - Timestamp generated at SFD for consistency and precision
   - Timestamp inserted into PTP packets on-the-fly during transmission
   - CRC and IPv6 UDP Checksum updated on-the-fly during packet transmission
7. Simplified design with expansion interfaces to support 1G/2.5G/5G/25G Ethernet
8. Similar design approach can be applied to 40G/100G+ Ethernet

### Usage Instructions

Subdirectories under project root:<br>
<blockquote>
esl: SystemC TLM platform and software design<br>
rtl: RTL design files<br>
tb: Testbench design files<br>
tc: Test case files<br>
sim: Simulation execution directory<br>
doc: Reference documents<br>
</blockquote>
<br>
RTL simulation is based on Linux OS using Icarus Verilog.<br>
Run basic RTL function test with following commands:<br>
<blockquote>
cd /path/to/sim<br>
./runcase.sh tc_rapid_ptp_test<br>
</blockquote>

For software design or co-design interest, refer to esl directory.<br>
Run hardware-software co-simulation with following commands:<br>
<blockquote>
cd /path/to/esl/solution <br>
mkdir build <br>
cd build <br>
cmake .. <br>
make <br>
./ptpv2_tlm <br>
</blockquote>
<br>
This open-source project focuses on establishing overall architecture design and open-source development processes. Currently, passing basic function tests is sufficient.<br>
If you want to further develop this project, please modify it according to your specific requirements and ensure adequate validation and testing.<br>

### IP Integration
When integrating XGE-PTPv2 IP into network systems:<br>
1. Place close to PHY layer, preferably within PHY near PCS layer
2. Use Full Duplex mode
3. To improve synchronization accuracy, disable any functions that may generate variable delay in the downstream direction of XGE-PTPv2 IP (e.g., 802.3az)
4. RTC working clock frequency should be equal or greater than data transfer clock frequency
5. Preferably use SyncE recovered clock as RTC working clock for highest synchronization accuracy

### Disclaimer
This design can be freely used without any fee from the author.<br>
IEEE1588-2008/2019 standards and solutions involved in the design may contain patent claims from certain organizations or individuals, which belong to the respective owners.<br>
The author makes no commitments regarding usage results and bears no legal liability arising from its use.<br>
Users must acknowledge and agree to the above statements. If not agreed, please do not use.<br>

### Follow Developer WeChat Official Account
To learn about project updates and join technical discussions, please search "时光之箭" on WeChat or scan the QR code below:<br>
![image](https://open.weixin.qq.com/qr/code?username=Arrow-of-Time-zd "时光之箭")