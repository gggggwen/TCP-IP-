# Chapter13. send & recv函数



## 13.1函数原型

```c++
recv函数用于socket通信中接收消息，接口定义如下：

ssize_t recv(int socket, void *buf, size_t len, int flags)
参数一：指定接收端套接字描述符；
参数二：指向一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
参数三：指明buf的长度；
参数四：一般置为0；
返回值：失败时，返回值小于0；超时或对端主动关闭，返回值等于0；成功时，返回值是返回接收数据的长度。
```

```c++

send函数用于socket通信中发送消息，接口定义如下：

ssize_t send(int socket, const void *buf, size_t len, int flags);  
参数一：指定发送端套接字描述符；
参数二：指明一个存放应用程序要发送数据的缓冲区；
参数三：指明实际要发送的数据的字节数；
参数四：一般置0；
返回值：失败时，返回值小于0；超时或对端主动关闭，返回值等于0；成功时，返回值是返回发送数据的长度。
```



## 13.2 TCP socket中的buffer

  每个TCP socket在内核中都有一个发送缓冲区和一个接受缓冲区，<font color =blue>**TCP的全双工工作模式以及TCP的流量和拥塞控制便依赖于这两个独立的buffer以及buffer的填充状态。**</font>

![img](https://i-blog.csdnimg.cn/blog_migrate/d4d376fa9bad19bd6d320058a3fd228f.png)

接受缓冲区把数据缓存入内核，如果没有调用read()系统调用的话，数据会一直缓存在socket的接受缓冲区内。不管进程是否调用recv()读取socket，对等端发来的数据都会经由内核接受并且缓存到socket的内核接受缓冲区之中。**recv()所做的工作，就是把内核缓冲区中的数据拷贝到应用层用户的buffer里面，并返回拷贝的字节数。（注意：<font color= red>是拷贝，不是像read那样读取之后，清空接受缓冲区内的数据。</font>）**

进程调用send()发送数据的时候，将数据拷贝到socket的内核发送缓冲区之中，然后返回拷贝的字节数。send()返回之时，数据不一定会发送到对等端去，**send()仅仅是把应用层buffer的数据拷贝到socket的内核发送缓冲区中，发送是TCP的事情。（注意：<font color =red>这里也是拷贝，不是像write那样发送之后，清空发送缓冲区内的数据。</font>）**

接受缓冲区被TCP用来缓存网络上接收到的数据，一直保存到应用进程读走为止。如果应用进程一直没有读取，接受缓冲区满了以后，发生的动作是：**接收端通知发送端，接收窗口关闭（win=0）**。这个便是滑动窗口上的实现。保证TCP套接口接受缓冲区不会溢出，从而保证了TCP是可靠传输。因为对方不允许发出超过所通告窗口大小的数据。这就是<font color = blubvvvvge>**TCP的流量控制，如果对方无视窗口大小而发出了超过窗口大小的数据，则接收方TCP将丢弃它**</font>












## 13.4 常见问题



在实际应用中,如果发送端是非阻塞发送,由于网络的阻塞或者接收端处理过慢,通常出现的情况是,发送应用程序看起来发送了10k的数据,但是只发送了2k到对端缓存中,还有8k在本机缓存中(未发送或者未得到接收端的确认).那么此时,接收应用程序能够收到的数据为2k.假如接收应用程序调用recv函数获取了1k的数据在处理,在这个瞬间,发生了以下情况之一,双方表现为:

- **发送应用程序认为send完了10k数据,关闭了socket:**

发送主机作为tcp的主动关闭者,连接将处于FIN_WAIT1的半关闭状态(等待对方的ack),并且,发送缓存中的8k数据并不清除,依然会发送给对端.如果接收应用程序依然在recv,那么它会收到余下的8k数据(这个前题是,接收端会在发送端FIN_WAIT1状态超时前收到余下的8k数据.), 然后得到一个对端socket被关闭的消息(recv返回0).这时,应该进行关闭.

- **发送应用程序再次调用send发送8k的数据:**

假如发送缓存的空间为20k,那么发送缓存可用空间为20-8=12k,大于请求发送的8k,所以send函数将数据做拷贝后,并立即返回8192;

假如发送缓存的空间为12k,那么此时发送缓存可用空间还有12-8=4k,send()会返回4096,应用程序发现返回的值小于请求发送的大小值后,可以认为缓存区已满,这时必须阻塞(或通过select等待下一次socket可写的信号),如果应用程序不理会,立即再次调用send,那么会得到-1的值, 在linux下表现为errno=EAGAIN.

- **接收应用程序在处理完1k数据后,关闭了socket: 接收主机作为主动关闭者,连接将处于FIN_WAIT1的半关闭状态(等待对方的ack).然后,发送应用程序会收到socket可读的信号(通常是 select调用返回socket可读),但在读取时会发现recv函数返回0,这时应该调用close函数来关闭socket(发送给对方ack);**

如果发送应用程序没有处理这个可读的信号,而是在send,那么这要分两种情况来考虑,假如是在发送端收到RST标志之后调用send,send将返回-1,同时errno设为ECONNRESET表示对端网络已断开,但是,也有说法是进程会收到SIGPIPE信号,该信号的默认响应动作是退出进程,如果忽略该信号,那么send是返回-1,errno为EPIPE(未证实);如果是在发送端收到RST标志之前,则send像往常一样工作;

以上说的是非阻塞的send情况,假如send是阻塞调用,并且正好处于阻塞时(例如一次性发送一个巨大的buf,超出了发送缓存),对端socket关闭,那么send将返回成功发送的字节数,如果再次调用send,那么会同上一样.

- **交换机或路由器的网络断开:**

接收应用程序在处理完已收到的1k数据后,会继续从缓存区读取余下的1k数据,然后就表现为无数据可读的现象,这种情况需要应用程序来处理超时.一般做法是设定一个select等待的最大时间,如果超出这个时间依然没有数据可读,则认为socket已不可用.

发送应用程序会不断的将余下的数据发送到网络上,但始终得不到确认,所以缓存区的可用空间持续为0,这种情况也需要应用程序来处理.

如果不由应用程序来处理这种情况超时的情况,也可以通过tcp协议本身来处理,具体可以查看sysctl项中的: `     net.ipv4.tcp_keepalive_intvl                       net.ipv4.tcp_keepalive_probes            net.ipv4.tcp_keepalive_time`





## 13.5 readv & writev