# Chapter 17 优于select的epoll

[toc]


##  17.1基于select的I/O多路复用技术性能相对劣势的原因



- <font sie =4>**内核中实现 select是用轮询方法，即每次检测都会遍历所有`FD_SET`中的句柄**</font>

 高并发的核心解决方案是1个线程处理所有连接的“等待消息准备好”，这一点上epoll和select是无争议的。但select预估错误了一件事，当数十万并发连接存在时，可能每一毫秒只有数百个活跃的连接，同时其余数十万连接在这一毫秒是非活跃的。select的使用方法是这样的：
$$
   返回的活跃连接 ==select（全部待监控的连接）。
$$
   什么时候会调用select方法呢？在你认为需要找出有报文到达的活跃连接时，就应该调用。所以，调用select在高并发时是会被频繁调用的。这样，这个频繁调用的方法就很有必要看看它是否有效率，因为，它的轻微效率损失都会被“频繁”二字所放大。它有效率损失吗？显而易见，<font color =blue>全部待监控连接是数以十万计的，返回的只是数百个活跃连接，这本身就是无效率的表现。</font>被放大后就会发现，处理并发上万个连接时，select就完全力不从心了。



- <font sie =4>**每次调用select前都需要向该函数传递监视对象的信息**</font>

在第十二章代码的循环体内部都会执行如下代码:

```c
int res ; 
temp = reads ;//每次轮询之前都需要进行拷贝,传递监视对象的信息
timeout.tv_sec =5;
timeout.tv_usec = 0;

res = select(1 , &temp , NULL , NULL , &timeout)
```

<font color =red>**每次调用select函数都需要向操作系统传递监视对象的信息**</font>,然而相较于`select`,    `epoll`只需要向操作系统传递一次见识对象, 监视范围或者内容发生变化时只通知发生变化的事项





附:(各I/O复用模型性能基准图)

![](./../phtoto/epoll1.jpg)





## 17.2实现select模型的关键三要素



### 	17.2.1 epoll高效的奥秘

   epoll精巧的使用了3个方法来实现select方法要做的事：

```c
 新建epoll描述符==epoll_create()
 epoll_ctrl(epoll描述符，添加或者删除所有待监控的连接)
 返回的活跃连接 ==epoll_wait（ epoll描述符 ）
```

   与select相比，epoll分清了频繁调用和不频繁调用的操作。例如，**epoll_ctrl是不太频繁调用的，而epoll_wait是非常频繁调用的**。这时，epoll_wait却几乎没有入参，这比select的效率高出一大截，而且，它也不会随着并发连接的增加使得入参越发多起来，导致内核执行效率下降。<font color =purple>epoll 特别适合 连接量大 但是活跃连接不多的情况</font>



   <font size=4>**要深刻理解epoll，首先得了解epoll的三大关键要素：mmap、红黑树、链表**。</font>



### 	17.2.2 mmap ,红黑树 ,双链表

- #### <font size =4>**mmap**</font>

 epoll是通过内核与用户空间mmap同一块内存实现的。mmap将用户空间的一块地址和内核空间的一块地址同时映射到相同的一块物理内存地址（不管是用户空间还是内核空间都是虚拟地址，最终要通过地址映射映射到物理地址），使得这块物理内存对内核和对用户均可见，减少用户态和内核态之间的数据交换。用户可以直接看到epoll监听的句柄，效率高。



- ####  红黑树

 	**作用：**

- **管理文件描述符**：红黑树用于存储和管理所有被注册到 `epoll` 实例中的文件描述符。每个文件描述符都作为红黑树的一个节点存在，<font color =red>节点包含了文件描述符的标识符和相关的事件信息（例如，是否对该文件描述符感兴趣的读写事件等）</font>。

​	  **特点**:

1. **自平衡**：红黑树是一种自平衡的二叉搜索树，保证了插入、删除和查找操作的时间复杂度为 O(log n)，其中 n 是树中节点的数量。
2. **高效的操作**：由于红黑树的平衡性质，`epoll` 在处理大量文件描述符时，能够高效地进行文件描述符的注册、删除以及事件的查询。



- #### `epoll` 模型中的双链表

1. **就绪列表的维护**:

   - 当用户调用 `epoll_ctl` 添加或修改文件描述符时，这些文件描述符会被存储在红黑树中，用于管理所有正在监听的文件描述符及其事件。
   - 当 `epoll` 监测到某个文件描述符上发生了感兴趣的事件（如数据可读或可写），内核会将这个文件描述符从红黑树中找到，并将其加入到一个双向链表中。这个链表就是所谓的“就绪列表”。

2. **事件通知 (`epoll_wait`)**:

   - 用户通过调用 `epoll_wait` 系统调用来等待事件发生。内核会遍历就绪列表，将所有在该时刻已就绪的文件描述符返回给用户。
   - 因为就绪列表是一个双向链表，`epoll_wait` 只需要遍历这个列表即可找到所有已经发生事件的文件描述符，而不必像 `select` 或 `poll` 那样遍历整个文件描述符集。
   - 由于双向链表的操作非常高效，尤其是在插入、删除操作方面是 O(1) 时间复杂度，所以当事件触发时，内核能够快速地将文件描述符插入就绪列表或从中移除。

   



##  17.3 实现epoll模型的三个函数

首先还是先介绍`epoll_event`结构体



### 		17.3.1 epoll_event 结构体

`epoll_event` 结构体的定义通常如下：

```c
struct epoll_event {
    __uint32_t events;  // 事件类型
    union {
        void *ptr;      // 用户自定义的数据指针
        __uint32_t u32;
        __uint64_t u64;
    } data;             // 事件相关的数据
};
```

- #### 成员解释：

1. **`events`**:
   - **类型**: `__uint32_t`
   - 说明: 这个字段用来指定用户感兴趣的事件类型。它是一个位掩码，包含了一个或多个事件标志。常见的事件标志见`17.3.3 epoll_ctl`

​     2.**`data`**:

- **类型**: 联合体（`union`）
- **说明**: 这个字段用来存储与事件相关的用户数据。它是一个联合体，因此可以用不同的方式来存储数据



### 	17.3.2 epoll_create

```c
#include<sys/epoll.h>

int epoll_create(int size);
//成功返回epoll的文件描述符 , 失败返回-1
//size: epoll实例的大小
```

注意, 调用epoll_create 创建的文件描述符保存空间称为`epoll例程` ,  **传递的size 是给予操作系统的一个例程大小的建议值,  换言之,size并非用来决定epoll例程的大小, 而是仅供重装系统参考**

同时, 程序结束后 , 也需要调用`close`关闭



### 	17.3.3 epoll_ctl

生成例程后，应在其内部注册监视对象文件描述符，此时使用 epoll_ctl 函数。

```c
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
/*
成功时返回 0 ，失败时返回 -1
epfd：用于注册监视对象的 epoll 例程的文件描述符
op：用于指定监视对象的添加、删除或更改等操作
fd：需要注册的监视对象文件描述符
event：监视对象的事件类型
*/
```



与其他 epoll 函数相比，该函数看起来有些复杂，但通过调用语句就很容易理解，假设按照如下形式调用 epoll_ctl 函数：

```
epoll_ctl(A,EPOLL_CTL_ADD,B,C);
```



第二个参数 EPOLL_CTL_ADD 意味着「添加」，上述语句有如下意义：

> epoll 例程 A 中注册文件描述符 B ，主要目的是为了监视参数 C 中的事件

再介绍一个调用语句。

```
epoll_ctl(A,EPOLL_CTL_DEL,B,NULL);
```



上述语句中第二个参数意味着「删除」，有以下含义：

> 从 epoll 例程 A 中删除文件描述符 B

从上述示例中可以看出，从监视对象中删除时，**不需要监视类型，因此向第四个参数可以传递为 NULL**

下面是第二个参数的含义：

- EPOLL_CTL_ADD：将文件描述符注册到 epoll 例程
- EPOLL_CTL_DEL：从 epoll 例程中删除文件描述符
- EPOLL_CTL_MOD：更改注册的文件描述符的关注事件发生情况

**epoll_event 结构体用于保存事件的文件描述符结合。**但也可以在 epoll 例程中注册文件描述符时，用于注册关注的事件。该函数中 epoll_event 结构体的定义并不显眼，因此通过调用语句说明该结构体在 epoll_ctl 函数中的应用。

```c
struct epoll_event event;
...
event.events=EPOLLIN;//发生需要读取数据的情况时
event.data.fd=sockfd;
epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&event);
...
```

#### 	17.3.3.1  epoll_event 结构体中events成员

上述代码将 epfd 注册到 epoll 例程 epfd 中，并在需要读取数据的情况下产生相应事件。接下来给出 epoll_event 的成员 events 中可以保存的常量及所指的事件类型。

- EPOLLIN：需要读取数据的情况
- EPOLLOUT：输出缓冲为空，可以立即发送数据的情况
- EPOLLPRI：收到 OOB 数据的情况
- EPOLLRDHUP：断开连接或半关闭的情况，这在边缘触发方式下非常有用
- EPOLLERR：发生错误的情况
- EPOLLET：以边缘触发的方式得到事件通知
- EPOLLONESHOT：发生一次事件后，相应文件描述符不再收到事件通知。因此需要向 epoll_ctl 函数的第二个参数传递 EPOLL_CTL_MOD ，再次设置事件。

可通过位或运算同时传递多个上述参数。



### 	17.3.4 epoll_wait

下面是函数原型：

```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
/*
成功时返回发生事件的文件描述符个数，失败时返回 -1
epfd : 表示事件发生监视范围的 epoll 例程的文件描述符
events : 保存发生事件的文件描述符集合的结构体地址值
maxevents : 第二个参数中可以保存的最大事件数
timeout : 以 1/1000 秒为单位的等待时间，传递 -1 时，一直等待直到发生事件
*/
```



该函数调用方式如下。需要注意的是，第二个参数所指缓冲需要动态分配。

```c
int event_cnt;
struct epoll_event *ep_events;
...
ep_events=malloc(sizeof(struct epoll_event)*EPOLL_SIZE);//EPOLL_SIZE是宏常量
...
event_cnt=epoll_wait(epfd,ep_events,EPOLL_SIZE,-1);
...
```

调用函数后，返回发生事件的文件描述符个数，同时在<font color =red>第二个参数指向的缓冲中保存发生事件的文件描述符集合。因此，无需像 select 一样插入针对所有文件描述符的循环。</font>





## 17.4 基于epoll模型的回声服务器

代码很简单原理几乎与select模型无异

```c
   struct epoll_event * epoll_events ;
    struct epoll_event event ; 
    int epfd , event_cnt ;

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1) error("bind error\n");
    if(listen(serv_sock ,5)==-1) error("listen error \n");

    epfd = epoll_create(EPOLL_SZ);
    epoll_events = malloc(sizeof(epoll_events)*EPOLL_SZ);
    
    event.data.fd = serv_sock;
    event.events = EPOLLIN;
    epoll_ctl(epfd , EPOLL_CTL_ADD , serv_sock , &event );

    int i ;
    while(1)
     {
        event_cnt = epoll_wait(epfd , epoll_events,EPOLL_SZ , -1);
        for (i = 0 ; i <event_cnt ; i++)
        {
           if( epoll_events[i].data.fd ==serv_sock) //说明发来了连接请求
           {
              sock_len = sizeof(clnt_addr);
              clnt_sock = accept(serv_sock , (struct sockaddr*)&clnt_addr , &sock_len);
              event.events = EPOLLIN ; 
              event.data.fd = clnt_sock ; 
              epoll_ctl(epfd , EPOLL_CTL_ADD , clnt_sock , &event);
              printf("connected client %d\n",clnt_sock);
           }
           else  //说明是客户端发来请求或者数据
           {
               mesg_len = read(epoll_events[i].data.fd,mesg,BUF_SZ);
               if(mesg_len==0)//说明发来的是断开连接的请求
               {
                  epoll_ctl(epfd , EPOLL_CTL_DEL,epoll_events[i].data.fd,NULL);
                  printf("connection closed\\n");
                  close(epoll_events[i].data.fd)  ;

               }
               else{
                   write(EPOLL_CTL_DEL,epoll_events[i].data.fd,mesg ,mesg_len);
               }
           }
        }
     }
     close (epfd);
     close(serv_sock);
     return 0 ;
```



## 17.5水平触发与边缘触发

**水平触发(level-trggered)**:

<font color= red>**默认情况下 , epoll模型选用水平触发**,(可以将上述示例,减小缓冲区`BUF_SZ=4` , 且在循环内部设置日志)</font>只要文件描述符关联的读内核缓冲区非空，有数据可以读取，就**一直**发出可读信号进行通知，

- 当文件描述符关联的内核写缓冲区不满，有空间可以写入，就**一直**发出可写信号进行通知

**边缘触发(edge-triggered)**:

​       只有当状态有改变时才会通知

- 当文件描述符关联的读内核缓冲区由空**转化为非空**的时候，则发出可读信号进行通知，
- 当文件描述符关联的内核写缓冲区由满**转化为不满**的时候，则发出可写信号进行通知

```C
int main()
{
    ........
    struct epoll_event event ; 
    struct epoll_event *epoll_events;
    epoll_events = malloc(sizeof(epoll_events)*EPOLL_SZ);
    int epfd ; int ep_cnt;
    
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1) error("bind error\n");
    if(listen(serv_sock ,5)==-1) error("listen error \n");

     event.data.fd =serv_sock;
     event.events = EPOLLIN;
     epfd = epoll_create(EPOLL_SZ);
     epoll_ctl(epfd , EPOLL_CTL_ADD, serv_sock,&event);

     int i ;
     while(1)
     {
        ep_cnt = epoll_wait(epfd , &epoll_events,EPOLL_SZ ,-1);
        for(i= 0 ; i<ep_cnt ;i++)
        {
            if(epoll_events[i].data.fd==serv_sock)
            {
                sock_len =sizeof(clnt_addr);
                clnt_sock = accept(serv_sock ,(struct sockaddr*)&clnt_addr,&sock_len);
                setnonblockingmode(clnt_sock);
                event.data.fd =clnt_sock;
                event.events = EPOLLIN|EPOLLET;
                epoll_ctl(epfd , EPOLL_CTL_ADD,clnt_sock,&event);
                printf("new connection established %d",clnt_sock);
            }
            else
            {
                while(1)
                {
                    mesg_len = read(epoll_events[i].data.fd , mesg, BUF_SZ);
                    if(mesg_len==0)
                    {
                        epoll_ctl(epfd , EPOLL_CTL_DEL,epoll_events[i].data.fd,NULL);
                        close(epoll_events[i].data.fd);
                        printf("connection closed\\n");
                        break;
                    }
                    else if(mesg_len <0)
                    {
                        if(errno ==EAGAIN)
                        {
                            break;
                        }
                    }
                    else{
                        write(EPOLL_CTL_DEL,epoll_events[i].data.fd ,mesg,mesg_len);
                    }
                    
                }
            }

        }
     }
     close(epfd);
     close(serv_sock);
}

void setnonblockingmode(int fd)
{
    int flag =fcntl(fd , F_GETFL, 0);
    fcntl(fd, F_SETFL,flag|O_NONBLOCK);
}
```



  

### 	17.5.1 为什么边缘触发模式需要将套接字改成非阻塞模式

**将连接套接字设置为阻塞模式的代码**

```c
{
   ....   
   while(1)
    {
       ....
       setnonblockingmode(clnt_sock);
    }
}
void setnonblockingmode(int fd)
{
    int flag =fcntl(fd , F_GETFL, 0);
    fcntl(fd, F_SETFL,flag|O_NONBLOCK);
}
```

阻塞模式会及时返回套接字缓冲区状态 :

- 若返回`-1`,且返回的`erono==EAGAIN`说明当前套接字内无数据
- 若返回`0`则说明客户端发出断开连接请求





### 17.5.2 为什么当套接字设置为非阻塞模式后,返回0为断开连接请求

在分析代码时, 我提出了如下的问题:

> **有没有可能会导致数据恰好在某个时间被接收完毕,即:在某个时间点mesg_len = 0; 从而导致程序误判成关闭套接字的信息?**

<font size=4>**详细解释:**</font>

1. **`read` 返回值为 `0` 的含义**:
   - 当 `read` 返回 `0` 时，表示连接的对端（客户端）已经关闭了连接（正常关闭）。这是一个标准的信号，表示没有更多的数据可以读取，并且连接已经结束。
   - 在这种情况下，服务器应该关闭对应的文件描述符并从 `epoll` 中移除这个套接字，正如你的代码中所做的那样。
2. **数据被完全接收时的情况**:
   - 在非阻塞模式下，如果数据已经被完全接收并且缓冲区中没有更多的数据，`read` 将返回 `-1`，并将 `errno` 设置为 `EAGAIN`。这是非阻塞 I/O 的常见模式，表示当前没有数据可读，程序应退出当前循环，等待下次有数据到达时再继续处理。
   - 在这种情况下，程序不会误判连接关闭，因为 `read` 的返回值不会是 `0`。
