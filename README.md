# scanner
remote network scanner for windows


--------------------------------------------------------------------------------------------------


扫描器，一个基本的工具，也是一个重要的工具。  

扫描分几十种类型，常见的是网络扫描。  
网络扫描又有很多种，我分两种：远程扫描和本地扫描。  

本文只关心：远程扫描。  
本地扫描（其实，这主要是本地的网络配置等信息）对于检测和防御有帮助。  
远程扫描善于攻击，虽然难度大，但更有用。  

远程扫描也分很多种，如：各个协议的，还有一个不可忽视的漏洞扫描。  

8:37 2021/11/21


--------------------------------------------------------------------------------------------------


设计目标：
1. 支持众多的参数选项，最好和知名的扫描器的选项兼容和一样。
2. 有配置文件。首选yaml,不建议ini.命令行选项覆盖配置文件。
3. 因为Windows不支持tcp raw.  
   所以也不建议用自己编写的驱动（因为签名），  
   所以建议用第三方的驱动，如：nmap的依赖的驱动。
4. 不考虑支持插件（不是DLL）。但是支持lua脚本.
5. 支持众多的协议。如：arp,ipv4/6,icmp,tcp,udp,http(s),smtp,rdp,ssh等。  
   不考虑支持，那些被淘汰的，不安全，不常用的协议，如：Ftp，Netbios，Telnet，igmp等。  
6. 支持漏洞扫描。
7. 输出格式支持XML，JSON，sqlite，当然还是可以重定向到文件。


--------------------------------------------------------------------------------------------------


设计实现：
1. 线程默认是64个，最多也是64个。
   如果嫌少，可以再来一级的循环嵌套64*64个线程。
   没有必要设置64*64个线程，除了增加CPU的调度，也不能增加网卡的性能。
   网卡性能有极限，这是它本身的性质决定的，而不是靠增加线程都能突破的。
   发送的线程64个，接受的线程也为64个。
   之所以是64，是因为WaitForMultipleObjects的第一个参数的极限是MAXIMUM_WAIT_OBJECTS（等于64）。
2. nmap和masscan.
3. GB-T20278-2013《信息安全技术网络脆弱性扫描产品安全技术要求》.


--------------------------------------------------------------------------------------------------
