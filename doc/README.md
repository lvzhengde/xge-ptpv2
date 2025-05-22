# XGE-PTPv2

# Basic Function Test
Run following commands to execute basic RTL function test<br>
cd /path/to/sim<br>
./runcase.sh tc_rapid_ptp_test<br>

# Design Specifications
1. RTC and TSU design are fundamentally separated, suitable for both switch/router applications and single-port MAC/PHY PTP designs
2. One-step clock, TC offload, and embedding ingress time into PTP packets will introduce processing delay in data path
3. External dis_ptpv2_i signal can bypass the entire design, also used to disable working clock signals
4. Configuration register bypass_dp signal can bypass data path while keeping TSU unit operational, suitable for two-step clock scenarios without additional data path delay
5. RTC precision can be adjusted by configuring tick_inc decimal point position

This open-source project focuses on establishing overall architecture design and open-source development processes. Currently, passing basic function tests is sufficient.<br>
If you want to further develop this project, please modify it according to your specific requirements and ensure adequate validation and testing.<br>

# Documentation
No formal documentation is planned currently. Developers will publish design details in technical articles through official WeChat account periodically.
To stay updated with project developments and design discussions, please follow our official WeChat account.

#### Follow Developer WeChat Official Account
To learn about project updates and join technical discussions, please search "时光之箭" on WeChat or scan the QR code below:<br>
![image](https://open.weixin.qq.com/qr/code?username=Arrow-of-Time-zd "时光之箭")