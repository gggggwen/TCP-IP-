# 

# 第五章.基于TCP/IP的服务器端和客户端

## 5.1TCP原理

###     5.1.1TCP套接字的i/o缓冲

当我们调用read()或者write()函数时,难道是直接从网络中输入或者提取输出的吗?答案并不是。

实际上,**当输入数据时,write()函数会将数据写入输出缓冲中,**并在某一个时刻会将数据发出,而并非write了多少就直接发到网络上。同理,当数据到达目标主机的目标端口号的套接字,**其实数据也会被写入到输入缓冲**并在某一时刻被读取。

![image-20240626155032988](C:\Users\32939\AppData\Roaming\Typora\typora-user-images\image-20240626155032988.png)

TCP套接字的io缓冲有如下 特性:

- 每个套接字会有自己的I/O缓冲区
- 在创建套接字时I/O缓冲区自动创建
- 在关闭套接字时,**输出缓冲会将数据发送到网络中**
- 在关闭套接字后,**将丢失输入缓冲中的数据**

###     5.1.2TCP连接的三次握手

![image-20240626212250562](C:\Users\32939\AppData\Roaming\Typora\typora-user-images\image-20240626212250562.png)

在图中: 

SYN 表示收发数据前的同步消息

我们假若Seq_Num初始为1000;在第一次挥手后,回传SYN+ACK ,此时ACK_Num=1001(表示客户端下次需要从序列1001数据开始传输)假若Seq_Num2=2000  那么第三次回收Ack_Num=2001

我的理解:

在实际网络编程中,在调用listen函数后,套接字被迫进入监听状态,并且会设置好队列,假设现在我们考虑仅有一个客户端向服务器发起连接请求,那么该连接请求会被放入连接队列,而accept()函数会在监听队列中抽取该连接,并立即为其建立专门为该连接实现交互的套接字(暂且成为连接套接字),同时发送ACK+SYN报文

```c
int accept(int serv_sock,struct sockaddr* ,stelen_t)
```

###   5.1.3TCP连接断开的四次挥手

   **客户端**                                                                                           **服务器端**

![image-20240626213417540](C:\Users\32939\AppData\Roaming\Typora\typora-user-images\image-20240626213417540.png)

详情见[TCP/IP面试秘籍](https://xiaolincoding.com/network/3_tcp/tcp_interview.html#%E7%AC%AC%E4%BA%8C%E6%AC%A1%E6%8C%A5%E6%89%8B%E4%B8%A2%E5%A4%B1%E4%BA%86-%E4%BC%9A%E5%8F%91%E7%94%9F%E4%BB%80%E4%B9%88)