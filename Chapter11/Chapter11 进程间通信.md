# Chapter 11 进程间通信

##  11.1 介绍pipe函数

- #### 函数原型:

```c++
#include<unistd.h>
int pipe( int filedes[2]);
//成功返回 0  , 失败返回-1
//filedes[0] --> 管道出口 用于接收数据
//filedes[1] --> 管道入口 用于传输数据
```





##  11.2 pipe实现单管道通信

为了完成进程间通信必须使用管道,而管道并非进程的资源 ,**同套接字一样,属于操作系统资源`非fork函数复制的对象`**,所以两个进程通过操作系统提供的空间进行通信 。 

所以说 两个进程可以一起共用两个接口 (管道出口与入口)

请看示例代码:如下代码展示了 父子进程共用同一个管道 (分别都实现了读写功能)

```c++
int fds[2];
char str1[]="who are u?";
char str2[]= "i am ur father";
pipe(fds);
if(pid == 0) //子进程
{
    write(fds[1],str1 , sizeof(str1));
    sleep(2);
    read(fds[0] , buf , BF_SZ);
    printf("from father mes : %s \n",buf);
}
else
{
    read(fds[0], buf ,BF_SZ );
    printf("from child mes :%s\n",buf);
    write(fds[1], str2 , sizeof(str2));
    sleep(3);
}
```

- **子进程中:sleep(2)的必要性数据进入管道后成为无主数据**,

​       因此 子进程将数据写入管道后 不能立刻去读取管道出口的数据 , 这样很可能自己拉的自己吃了,并且这样还导致父进程陷入阻塞状态,无限期等待数据传输过来!! 需要一点点时间 , 先让父进程收到数据 ,并将要写入的数据传输到管道内。

- **父进程中sleep(3)的必要性 ,防止先终止弹出命令提示符**,

  ![image-20240714204523233](./../phtoto/image-20240714204523233.png)