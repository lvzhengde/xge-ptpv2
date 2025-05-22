# XGE-PTPv2 Software Design based on SystemC TLM Platform

#### Usage Instructions
The design and testing are based on Linux OS, requiring installation of Verilator, SystemC, and CMake.<br>
CMake can be installed directly through Linux package manager, while Verilator and SystemC installation instructions can be found on their official websites or developer articles from this project.<br>
<br>
To run simulations, follow these steps:<br>
cd /path/to/solution <br>
rm -rf build && mkdir build && cd build <br>
cmake .. <br>
make <br>
./ptpv2_tlm <br>
<br>
For debugging, install Visual Studio Code and open the ESL project directory at xge-ptpv2/esl.<br>
<br>
For detailed simulation testing procedures, please refer to the following document:<br>
/path/to/xge-ptpv2/esl/doc/Design_Testing_Description.pdf<br>
<br>
In SyncE mode, simulation results show the protocol algorithm can achieve high-precision time and frequency synchronization between PTP Slave and Master within 1 second.<br>
In non-SyncE mode, simulation results show the Protocol and Clock Servo algorithms can achieve time and frequency synchronization within about 15 seconds.<br>
Regarding simulation duration: on the developer's machine, SyncE simulation takes about 20 minutes while non-SyncE simulation requires over 6 hours.<br>

#### Disclaimer
This design can be freely used without any fee from the author.<br>
IEEE1588-2008/2019 standards and solutions involved in the design may contain patent claims from certain organizations or individuals, which belong to the respective owners.<br>
The author makes no commitments regarding usage results and bears no legal liability arising from its use.<br>
Users must acknowledge and agree to the above statements. If not agreed, please do not use.<br>

#### Follow Developer WeChat Official Account
To learn about project updates and join technical discussions, please search "时光之箭" on WeChat or scan the QR code below:<br>
![image](https://open.weixin.qq.com/qr/code?username=Arrow-of-Time-zd "时光之箭")