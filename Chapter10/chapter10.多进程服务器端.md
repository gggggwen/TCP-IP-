# Chapter10.多进程服务器



##    10.1 fork()函数

```c++
#include<unistd.h>

pid_t fork(void);
//成功返回*子进程id* 失败返回 -1
```

- 父进程: fork返回子进程id

- 子进程 : fork  返回0

  ```c++
  int pid =fork();
  if(pid <0)
  {   printf("error!\n");
      exit(1);
  }
  else if(pid == 0)
  {
      //子进程代码
  }
  else
  {
      //父进程代码
  }
  ```

###  10.1.2fork函数调用机制:

fork函数调用,后父进程复制出子进程 , **父进程和子进程占用不同的内存空间,变量的生命周期仅存在于自己的进程之中,**



---



##  10.2  wait(),waitpid()---预防僵尸进程



###    10.2.1如果在子进程结束后 不及时wait会怎样?

-  **子进程结束,父进程还正在运行**,子进程会停留在僵尸状态(zombie),此时子进程没有任何运行,却占用了内存资源
- **子进程结束,父进程也马上结束**,因为操作系统不允许没有父进程的子进程,因此在运行结束后,**进入僵尸状态的子进程会转交给init进程 ,随后子进程被销毁**。    注意: 子进程不能被kill



###    10.2.2 wait()函数

```c
#include<sys/wait.h>

pid_t wait(int* statloc);
//成功返回终止的子进程的id 失败返回-1
```

子进程终止,传递返回值将保存到函数的参数所指向的内存空间。但函数参数指向的内存空间还有其他信息,因此需要**宏进行分离**

- WIFEXITED 子进程正常结束返回true
- WEXITSTATUS 返回子进程的返回值

```c++
int status ;
printf("child process returned : %d",WEXITSTATUS(status));
```

- #### 使用wait需谨慎!

  如果在使用wait时,无终止子进程**,那么父进程将进入阻塞状态,**直至有子进程终止



###     10.2.3 waitpid()函数

```c
#include<sys/wait.h>
pid_t waitpid(pid_t pid , int*statloc , int options);
```

- pid : 等待目标子进程的pid 如果传入-1 和wait差不多
- 同wait
- 可以传递头文件sys/wait.h 中声明的常量 WNOHANG ,即使无终止子进程,不会进入阻塞状态



## 10.3 信号处理

- #### 信号处理机制(Signal Handling)

  此处的''信号''是在特定情况下,操作系统向进程发送的消息。 另外,为了响应该消息,执行与消息相关的自定义操作过程被称为'处理'或'信号处理'



###    10.3.1 signal()函数（因为环境原因，不太稳定）

```c++
#include<signal.h>
void (*signal(int signo , void(*handler)(int)))(int);
```

- 函数名:signal
- 参数: int signo 以及函数指针 void(*handler)(int)

- 第一个参数signum：指明了所要处理的信号类型，它可以取除了SIGKILL和SIGSTOP外的任何一种信号。 　 第二个参数handler：描述了与信号关联的动作。

- 此函数必须在signal()被调用前申明，handler中为这个函数的名字。当接收到一个类型为sig的信号时，就执行handler 所指定的函数。


- (int）signum是传递给它的唯一参数。执行了signal()调用后，进程只要接收到类型为sig的信号，不管其正在执行程序的哪一部分，**就立即执行handler()函数**。


- 当handler()函数执行结束后，控制权返回进程被中断的那一点继续执行。



###     10.3.2 重要的信号参数(int signo)

- **SIGALRM: 已经到达alarm函数注册的时间**
- **SIGINT :通过获取键盘Ctrl+C 而产生的信号**
- **SIGCHLD: 因子进程终止而产生的信号**



###     10.3.3 alarm()函数

```c++
unsigned int alarm(unsigned int seconds);
//返回值为 0 或者以秒为单位距SIGALRM 信号发生的剩余时间
```

- 功能与作用：alarm()函数的主要功能是设置信号传送闹钟，即用来设置信号SIGALRM在经过参数seconds秒数后发送给目前的进程。

- 如果未设置信号SIGALARM的处理函数，那么alarm()默认处理终止进程。

- 函数返回值：如果在seconds秒内再次调用了alarm函数设置了新的闹钟，则后面定时器的设置将覆盖前面的设置，即之前设置的秒数被新的闹钟时间取代；

- 当参数seconds为0时，之前设置的定时器闹钟将被取消，并将剩下的时间返回

**常见示例timeout函数**

```c++
void timeout(int sig)
{
    if(sig == SIGALRM)
    {
        puts("alarm!!\n");
        alarm(2);//重新注册时间
    }
    else
    {
        puts("incorrect signa!\n");
    }
}
}
```



###       10.3.4 sigaciton（）函数（较为稳定，所有操作系统上面无差别）

```c++
#include<signal.h>
int sigaction(int signo , const struct sigaction*act , struct sigaction* old_act);

struct sigaction act
{
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
}
```

本阶段初始化act.sa_mask ,act.sa_flags 皆为0 即可

```c++
struct sigaction act;
act.sa_handler = func ;
sigemptyset(&act.sa_mask);
act.sa_flags = 0 ;
```





###  10.4 实子进程现杀死僵尸
