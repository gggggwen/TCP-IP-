# Chapter 14 多播和广播

没啥好说的 , 就记一些基本知识吧

## 14.1 多播地址

###  14.1.1IPv4 多播地址(D类IP地址)

IPv4 的多播地址范围是 `224.0.0.0` 到 `239.255.255.255`。在这个范围内：

- **224.0.0.0 - 224.0.0.255**: 保留用于本地网络控制块（Local Network Control Block）。
- **224.0.1.0 - 224.0.1.255**: 保留用于 Internet Group Management Protocol（IGMP）。
- **224.0.2.0 - 238.255.255.255**: 通用的 IPv4 多播地址范围，可以用于一般的多播通信。
- **239.0.0.0 - 239.255.255.255**: 用于私有多播地址。

在通用的 IPv4 多播地址范围 `224.0.2.0` 到 `238.255.255.255` 内，可以**<font color = redldxa>自定义多播地址，但需要避免使用已经被其他协议或应用程序占用的特定地址</font>。**



###    14.1.2IPv6 多播地址

IPv6 的多播地址范围是 `FF00::/8`。在这个范围内，`FF` 开头的地址都用于多播。

IPv6 多播地址可以自定义，但需要注意以下几点：

- **FF0x::/16**: 这个范围是用于预定义的多播地址，例如全局节点地址、全局路由器地址等。

- **FF1x::/16**: 这个范围是为本地链路多播保留的。

- FF2x::/16 - FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF

  : 这部分是可用于自定义的 IPv6 多播地址。

​       

```c++
   struct sockaddr_in mul_adr           //多播地址的命名
```



## 14.2 多播

多播是基于UDP协议完成的 , 因此 多播数据包和UDP数据包大体一样 ,但是不同的是: 多播数据包经过路由器后会复制该数据包并传递给多个主机 

因此, 也值得注意的是 **多播需要依赖路由器来完成**

适用场景---多媒体数据的实时传输



###     14.2.1 TTL(Time to Live)的设置

```c++
int send_sock ;
int time_live = 64 ; //经过64个路由器后自动被销毁

send_sock = socket(AF_INET , SOCK_DGRAM , 0);
setsockopt(send_sock , IPPROTO_IP , IP_MULTICAST_TTL , (void*)&time_live ,
          sizeof(time_live));
```



###    14.2.2 设置多播的套接字选项

```c++
int recv_sock ;
struct ip_mreq join_addr ;

join_adr.imr_multiaddr,s_addr =; //多播地址IP
join_adr.imr_interface.s_addr = ;"加入多播组的主机地址信息"
setsockopt(recv_sock , IPPROTO_IP , IP_ADD_MEMBERSHIP,(void*)&join_addr , 
          sizeof(join_addr));
```



下面介绍 `struct ip_mrep`

```c++
struct ip_mreq 
{
    struct in_addr imr_multiaddr ; //组的ip地址
    struct in_addr imr_interface ;//自己的ip ,可以用INADDR_ANY;
}
```



## 14.3 广播

同样也是基于UDP传输

分为:

- 直接广播(Directed broadcast)

  直接广播的ip地址除网络地址以外 ,其他全部设为1 , **可采用直接广播的方式向<font color = red>特定区域内</font>的所有主机传输数据**

- 本地广播(Local broadcast )

  本地广播使用的IP地址限定为`255.255.255.255` 假若主机ip`192.168.101.222`采用本地广播,会向`192.168.101`网络中所有主机传输数据

```c++
int bcast =1 ;//对该变量初始化为1 来将SO_BROADCAST 选项信息设置为1
int send_sock;
send_sock = socket(AF_INET , SOCK_DGRAM ,0);
setsockopt(send_sock , SOL_SOCKET , SO_BROADCAST , (void*)&bcast , sizeof(bcast));
```

