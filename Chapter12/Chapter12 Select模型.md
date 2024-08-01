# Chapter12 IO复用--select模型



##  12.1 struct fd_set

```c
struct fd_set
{
    unsigned long fds_bits[__FD_SETSIZE / (8 * sizeof(unsigned long))];
}
```

这个定义说明了 `fd_set` 是一个包含了一个位图数组 `fds_bits` 的结构体，用于表示文件描述符集合。每一个文件描述符对应位图中的一个位，位的值（0 或 1）表示该文件描述符是否在集合中。

且由于fd_set结构体大小为1024 bit 因此**select 模型可以接受1024 个连接**



以下是一些常用的操作 `fd_set` 的宏：

- #### `FD_ZERO(fd_set* fdset)  `   初始化文件描述符集合，将集合清空。

  **<font color=red>注意:如果不进行该初始化操作,将有可能导致以下后果:</font>**

  **未定义的初始值**：

​       `fd_set`结构体中的位图阵列`fds_bits`将包含未定义的值。这些值可能是内存中先前存储的数据，具体值是不确定的。

​          这意味着`FD_ISSET`、`FD_SET`操作`FD_CLR`可能会在未定义的状态下工作，导致不可预测的行为。

​       **潜在的错误和不稳定性**：

​          由于`fds_bits`中的位可能是随机的，`select`函数可能会认为某些文件回调位于集合中，即使你没有明确添加它们。

​          这可能会导致`select`函数返回错误的结果，导致您的程序尝试在未准备好的文件读写上进行操作，从而引发读取/读取错误、崩溃或其他不可预期的行为。

- #### `FD_SET(int fd , fd_set* fdset)`    将文件添加到集合中

  需要监测多少个文件描述符 就需要添加多少次

- #### `FD_CLR(int fd ,fd_Set* fdset)`      从集合中移除一个文件描述符。

- #### **`FD_ISSET(int fd ,fd_Set* fdset)`** : 检查一个文件描述符是否在集合中。



## 12.2  select 函数

```c
#include<sys/select.h>
#include<sys/time.h>

int select(  int maxfd, fd_set *readset, fd_set *writeset, fd_set* exceptset,
      struct timeval *timeout); 
//maxfd -->监测文件描述符的数量 一般需要在最大文件描述符基础上+1
//readset -->关注文件描述符是否存在待读取数据
//.....
//.....
//timeout -->设置超时 ,若传递NULL 则表明无限等待事件发生
//返回值: 0 超时 -1 错误  >0 发生事件的文件描述符的数量

struct timeval
{
    int tv_sec ;
    int tv_usec ; //微秒
}

```

- #### 读事件

  1)从已连接队列中有已经准备好的socket

  2)接收缓冲区中有数据可读

  3)tcp连接断开(对端调用close()函数)

- #### 写事件

  输入缓冲区未满 可以写入数据

  

## 12.3 标准输入作为select函数的示例

```c
#include<sys/time.h>
#include<sys/select.h>
#include<stdio.h>
#include<stdlib.h>
#include<

int main()
{
    FD_ZERO(reads); 
    FD_SET(0,reads) ;//将标准输入注册到reads中
    
    while(1)
    {
        int res ; 
        temp = reads ;//每次轮询之前都需要进行拷贝
        timeout.tv_sec =5;
        timeout.tv_usec = 0;
        
        res = select(1 , &temp , NULL , NULL , &timeout)
        if(res ==-1) 
        {.......}
        else if(res == 0)
        {.......}
        else 
        {
            if(FD_ISSET(0,&temp)) //若存在读事件
            {
                //将数据打印到命令行
            }
        }
    }
}
```



## 12.4 select模型的一些弊端
