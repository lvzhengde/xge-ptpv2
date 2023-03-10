# XGE-PTPv2

#### 介绍
用于10G以太网的PTPv2硬件引擎设计，Verilog HDL描述

#### 设计特点 

1.  内含一个高精度的RTC，支持PPS输出和PPS输入 
2.  完整支持IEEE Std 1588-2008/2019标准所描述的One-step Clock 和Two-step Clock两种工作模式
3.  Delay Request-Response机制和Peer Delay机制均支持
4.  支持各种封装格式的IEEE1588v2报文
    - PTP over IEEE802.3/Ethernet， 因此支持802.1AS
	- PTP over UDP IPv4/IPv6
	- No VLAN/Single VLAN/Double VLAN
5.  支持Transparent Clock的完全硬件实现（TC Hardware Offload）
6.  精确高效的时间戳处理
    - 固定在SFD处产生时间戳，保持一致性和精确性
	- 在传输过程中（on-the-fly）插入时间戳至PTP报文
	- 在传输过程中（on-the-fly）更新报文的CRC及IPv6 UDP Checksum
7.  设计尽量简化并且预留扩展适配接口，以支持1G/2.5G/5G/20G以太网等

#### 使用说明

项目根目录下几个子目录的内容如下:<br>
<blockquote>
esl: SystemC TLM平台及软件设计
rtl: RTL设计文件<br>
tb: 测试平台设计文件<br>
tc: 测试用例文件<br>
sim: 仿真运行所在目录<br>
doc: 参考文档<br>
</blockquote>
<br>
执行以下命令以运行基本的RTL功能测试<br>
<blockquote>
cd /path/to/sim<br>
./runcase.sh tc_rapid_ptp_test<br>
</blockquote>
<br>
本开源项目着重于整体架构设计和开源设计流程的建立，目前情况下，保证基本功能测试通过即可。<br>
如果读者有意在此基础上进一步开发，请自行进行修改，使其功能符合特定需求并得到充分的验证和测试。<br>

#### 免责声明

本设计可以自由使用，作者不索取任何费用。<br>
IEEE1588-2008/2019标准以及设计中涉及到的解决方案可能隐含有一些机构或者个人的专利诉求， 则专利权属于相关的所有者。<br>
作者对使用结果不做任何承诺也不承担其产生的任何法律责任。<br>
使用者须知晓并同意上述声明，如不同意则不要使用。<br>

#### 关注开发者公众号
如果需要了解项目最新状态和加入相关技术探讨，请打开微信搜索公众号"时光之箭"或者扫描以下二维码关注开发者公众号。<br>
![image](https://open.weixin.qq.com/qr/code?username=Arrow-of-Time-zd "时光之箭")



