### snifferview-封包嗅探分析工具
码云git地址：https://gitee.com/lougd/snifferview.git

#### 开发记录:
  这个工具的第一个版本是2015年做的，因为工作中经常要在不同的终端PC上抓包分析网络协议， 一直用的wireshark进行抓包分析，wireshark的安装包比较大，每次装起来太繁琐，也不支持回环网络封包的捕获，于是就起意自己做一个自己的封包协议分析工具。于是就有了这个工具的第一个版本。然后一边使用一边丰富工具的功能，就有了现在的这个版本。

#### 主要功能及特点:

  该工具不是一个demo，是一个稳定，切实可用，功能比较完善的网络封包修改，分析工具，如果需要经常进行网络封包的捕获，分析，记录可以使用这个工具。
1. 支持本地所有网卡的网络封包嗅探捕获，包括127地址的回环包。
2. 支持类似wireshark的封包过滤语句，方便灵活的对网络封包进行过滤。
3. 支持tcp流追踪功能，方便的跟踪单个的tcp数据流。
4. 支持进程网络状态探测，可以方便的查看指定进程的网络状态。
5. 支持已捕获网路数据的导入导出，可以方便的随时dump已捕获的网络数据。
6. 工具小巧，绿色（1mb多，只有一个可执行文件），无任何依赖，启动运行速度快。
7. C++语言开发，vs2008集成工具开发工具编译，用vs2008打开工程根目录下的SnifferView.sln工程文件编译即可。

#### 工具功能截图:

![主界面](https://images.gitee.com/uploads/images/2019/0922/113913_ebbcaf79_498054.png "1.png")
![tcp流追踪界面](https://images.gitee.com/uploads/images/2019/0922/113932_1ef08d36_498054.png "2.png")
![查看网路状态界面](https://images.gitee.com/uploads/images/2019/0922/114041_80e035a5_498054.png "3.png")

#### 软件架构


```
ComLib子工程：
基础功能静态库，给各个子功能提供通用的基础功能组件。
SnifferView子工程：
主功能模块，实现主要的功能，包括网络封包的嗅探，过滤语句编译解析器实现，网络协议的分析，数据流查看等等。
Dumper子工程：
异常捕获处理模块，捕获未处理异常并生成异常dump和异常日志，用于分析异常原因。
SyntaxView子工程：
语法高亮控件，基于Scintilla改造而成，目前用于展示tcp数据流数据。
```

#### 使用说明

```
本程序只有一个可执行程序，程序启动后即开始封包嗅探，但这时由于没有过滤语句，捕获到的封包会有很多，可以在主界面的过滤规则框中输入合适的过滤规则来捕获感兴趣的网络封包。具体的过滤规则和wireshark的类型，又做了一些优化，主要的规则如下。

SnifferView过滤规则：
SnifferView的过滤语法举例：
ip.addr==192.168.168.231   过滤ip源地址或者目标地址为192.168.168.231的网络封包
icmp                       过滤icmp协议的封包
tcp.src==8345              过滤tcp源端口为8345的网络封包
tcp.length>128             过滤长度大于128字节的tcp封包，这个长度刨除ip头和tcp头 
tcp contains "GET"         过滤内容中包含GET字符串的tcp封包
tcp[4:n32]==0x12ff         过滤tcp用户数据偏移4字节取一个32位数据大小为0x12ff的封包
tcp.flag.syn               过滤有syn标记的tcp封包
tcp[chars]=="GET"          过滤tcp用户数据偏移O字节为GET的封包
http                       过滤http协议
http.get                   过滤http的get协议
http.post                  过滤http post协议
http.resp                  过滤http的返回包
http.url contains "img"    过滤http的url中包含img字符串的封包

过滤规则的关键字：
ip层：ip, ip.addr, ip.src, ip.dst, ip.length

icmp层：icmp
tcp层：tcp.port, tcp.src, tcp.dst, tcp.length，tcp.flag.syn|ack|fin|rst|psh|urg, tcp[a:b](a为偏移，b是具体的数据类型，没有a的意思是偏移为0)， tcp contains “aaa”(包含字符串aaa的tcp封包)

udp层：和tcp类似，不再赘述 

应用层协议：
http协议：http, http.get, http.post, http.head, http.options, http.put, http.delete, http.tarce, http.url contains "aaa"(http的url包含aaa的封包)
过滤规则的数据类型（用于tcp[a:b]或者udp[a:b]）：
n8（8位无符号整型），n16（16位无符号整型），n32（32位无符号整型），byte（同n8），bytes（byte列表），char（字符型），chars（字符串）

过滤规则的逻辑连接符：
>（大于）, <（小于）, >=（大于等于）, <=（小于等于）, ==（等于）, !=（不等于）, &（按位与）

各个过滤表达式可以用and(&&)或者or(||)进行连接，可以使用小括号 比如获取长度大于128的GET包或者POST包:
tcp.length>128 and (tcp[chars]==”GET” or tcp[chars]== “POST”)

SnifferView语法相对wireshark的主要改进:
wireshark过滤封包内容的时候语法是这样的tcp[a:b]，a为偏移，b为匹配的长度，偏移是从tcp头开始的，并且tcp头还是变长的，并且只能按字节逐个匹配，用起来很不方便，我们通常关注的并不是tcp头中的内容，而是用户数据的内容，因此SnifferView过滤内容的时候偏移是从用户数据开始的，程序自动计算tcp头的长度，支持各种的数据类型的匹配，自动计算匹配的长度，如果匹配的字符串里有\n\r将自动换为回车和换行。
比如：
tcp[4:n16]==0x33ee             从用户数据偏移4字节匹配一个16位整型数据，大小为0x33ee 
tcp[5:chars]=="aaaa"            从用户数据偏移5个字节匹配一个字符串aaaa

关于配置界面里的主机字节序和网络字节序：
选主机字节序的意思是封包里的数据是以主机字节序的方式存储的，选网络字节序的意思是封包里的数据是以网络字节序存储的。
比如：
tcp[4:n32]==0x12345678
如果选的是主机字节序会从用户数据偏移4字节的地方开始依次匹配0x78,0x56,0x34,0x12
如果选的是网络字节序会从用户数据偏移4字节的地方开始依次匹配0x12,0x34,0x56,0x78
```


