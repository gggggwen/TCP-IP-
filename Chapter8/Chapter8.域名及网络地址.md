# Chapter8.域名及网络地址



##    8.1  域名解析系统

  **DNS 域名解析系统（Domain Name System）** 是进行域名（domain name）和与之相对应的 IP 地址（IP address）转换的服务器。DNS 中保存了一张域名（domain name）和与之相对应的 IP 地址（IP address）的表，以解析消息的域名

一种**分布式的查询系统**

##   8.2 查询类型

### 递归查询

递归查询（Recursive）是一种 DNS 服务器的查询模式，在该模式下 DNS 服务器接收到客户机请求，必须使用一个准确的查询结果回复客户机。如果 DNS 服务器本地没有存储查询 DNS 信息，那么该服务器会询问其他服务器，并将返回的查询结果提交给客户机。所以，一般情况下服务器跟内网 DNS 或直接 DNS 之间都采用递归查询。

### 迭代查询

DNS 服务器另外一种查询方式为迭代查询，DNS 服务器会向客户机提供其他能够解析查询请求的 DNS 服务器地址，当客户机发送查询请求时，DNS 服务器并不直接回复查询结果，而是告诉客户机另一台 DNS 服务器地址，客户机再向这台 DNS 服务器提交请求，依次循环直到返回查询的结果。所以一般内网 DNS 和外网 DNS 之间的都采用迭代查询。

## 分层结构

域名是分层结构，域名 DNS 服务器也是对应的层级结构。有了域名结构，还需要有域名 DNS 服务器去解析域名，且是需要由遍及全世界的域名 DNS 服务器去解析，域名 DNS 服务器实际上就是装有域名系统的主机。域名解析过程涉及 4 个 DNS 服务器，分别如下：

| 分类           | 作用                                                         |
| -------------- | ------------------------------------------------------------ |
| 根 DNS 服务器  | Root NameServer，本地域名服务器在本地查询不到解析结果时，则第一步会向它进行查询，并获取顶级域名服务器的 IP 地址。 |
| 顶级域名服务器 | TLD（Top-level） NameServer。负责管理在该顶级域名服务器下注册的二级域名，例如 `www.example.com`、`.com` 则是顶级域名服务器，在向它查询时，可以返回二级域名 `example.com` 所在的权威域名服务器地址 |
| 权威域名服务器 | Authoritative NameServer。在特定区域内具有唯一性，负责维护该区域内的域名与 IP 地址之间的对应关系，例如云解析 DNS。 |
| 本地域名服务器 | DNS Resolver 或 Local DNS。本地域名服务器是响应来自客户端的递归请求，并最终跟踪直到获取到解析结果的 DNS 服务器。例如用户本机自动分配的 DNS、运营商 ISP 分配的 DNS、谷歌/114 公共 DNS 等 |

> 每个层的域名上都有自己的域名服务器，最顶层的是根域名服务器
>
> 每一级域名服务器都知道下级域名服务器的 IP 地址，以便于一级一级向下查询



##   8.3 ip与域名之间的互换

###     8.3.1 利用ip查域名

```c
#include<netdb.h>
struct hostent* gethostbyname(const char *hostname)
//成功返回hostent地址 ,失败返回空指针
```

这个函数使用方便，只要传递字符串，就可以返回域名对应的IP地址。只是返回时，地址信息装入 hostent 结构体。此结构体的定义如下：

```c++
struct hostent
{
    char *h_name;       /* Official name of host.  */
    char **h_aliases;   /* Alias list. 别名 */
    int h_addrtype;     /* Host address type.  */
    int h_length;       /* Length of address.  */
    char **h_addr_list; /* List of addresses from name server.  */
}
```

- h_addr_list：这个是最重要的的成员。通过此变量以整数形式保存域名相对应的IP地址。另外，用户比较多的网站有可能分配多个IP地址给同一个域名，利用多个服务器做**负载均衡**，。此时可以通过此变量获取IP地址信息。

  

###  8.3.2 遍历域名查询结果*

```c
int i = 0; 
while(host->hostent[i]!=NULL)
 {
     printf("%s \n" , host->hostent[i]);
     i++
}
int j =0;
while(host->h_addr_list[j]!=NULL)
{
    printf("ip addr:　%s",inet_atoi(*(struct in_addr* )host->h_addr_list[j]));
    j++
}
 
```

- 注意看打印ip地址的printf细节:
  1.  首先类型强转: 原指针为字符串指针,类型强转为struct in_addr*(其实可看做32位整型指针)  
  2.  为什么要类型强转?  因为inet_atoi要求传入参数为in_addr类型,因此需要对其进行解引用,,若直接对字符串指针进行解引用**解引用后**inet_atoi只会**访问整个字符串的第一个字节**,因为该类型指针大小仅为1字节!!!!

### 8.3.3 void* 与char*

当指针指向