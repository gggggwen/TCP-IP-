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



## 10.3 信号处理**

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

- **为什么需要传入act的地址**?

  



###  10.3.4 实现杀死僵尸子进程



```c++
void sig_handler(int sig)
{
    if(sig ==SIGCHLD)
    {
        int status ;
        waitpid(-1, &status, WNOHANG);
        if(WIFEXITED(status))
        {
            printf("the child proc returned %d\n",(WEXITSTATUS(status))); //日志
        }
    }
}

int main()
{
    //在初始化阶段设置好sigaction 
    pid_t pid ; 
    struct sigaction act ;
    act.sa_handler = sig_handler;
    act.sa_flags = 0; //default
    sigemptyset(act.sa_mask);
    
    pid = fork();
    if(pid == 0) //child
    {
        ...
    }
    else if(pid > 0 ) //father
    {
        ...
    }
}
```

- 通过sigaction函数, **忽略了SIGCHLD信号**`(此处忽略并不是不处理的意思)`,并带来如下效果:
  - 当一个子进程终止时，如果父进程忽略了 `SIGCHLD` 信号，**子进程会自动被回收**（不会成为僵尸进程）。这意味着子进程的终止状态会被立即丢弃，系统不会保留其信息供父进程通过 `wait()` 或 `waitpid()` 调用获取。
  - 这种方式可以**防止父进程必须显式地调用** `wait()` 或 `waitpid()` 来清理子进程资源。



## 10.4 实现基于多进程的并发服务器

- **初始化阶段**

```c++
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	
	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	state=sigaction(SIGCHLD, &act, 0); //忽略子进程终止信号,使得父进程不会因为需要wait而阻塞
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));//一定要记得memset初始化所有字节值都为0
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

```



- **封装并将套接字进入监听状态**

```c++
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
```



####      10.4.3**服务器客户端交互过程**(太重要了)

父进程负责监听 子进程负责连接传输

```c++
	while(1)
	{
		adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		if(clnt_sock==-1)
			continue;
		else
			puts("new client connected...");
		pid=fork();
		if(pid==-1)
		{
			close(clnt_sock);
			continue;
		}
		if(pid==0)
		{
			close(serv_sock);
			while((str_len=read(clnt_sock, buf, BUF_SIZE))!=0)
				write(clnt_sock, buf, sr_len);
			
			close(clnt_sock);
			puts("client disconnected...");
			return 0;
		}
		else
			close(clnt_sock); //父进程需要关闭连接套接字
	}
	close(serv_sock);
	return 0;
```

- 首先说明一下有关accept报错处理:

  ```c++
  clnt_sock= accept(serv_sock , (struct sockaddr*)&clnt_addr,addr_Sz);
  if(clnt_sock==-1)
      continue; 
  
  ```

  为什么不进行日志处理,标记报错? 计算机while循环处理很快。 在大多数时间段计算机不会收到任何连接请求,如果说此时仅进行打印报错不跳过 ,**后续程序还会继续创建进程,可是进程的任务是输入输出数据的,你的连接都没建立,怎么传输?**因此当clnt_sock =-1 时需要调用continue



- 其次是关于进程创建

  ```c++
  if(pid == -1)
  {
      continue;
  }
  if(pid==0)
  {
      .....
  }
  else 
  {
      .....
  }
  ```

  可以学一下这种模式



- 最后是关于数据读写

  ```
  if(pid ==0)
  {
    close(serv_sock); //关闭监听套接字避免占用过多系统资源
    int str_len ;
    while((str_len = read(clnt_sock , mesg ,BUF_SZ))!=0)
    {
       write(clnt_sock, buf, sr_len);
    }
    puts("connection disconnected\n");
    close(clnt_sock);
  }
  ```

  



### 10.5  实现具有I/O分割程序的客户端

原理很简单: 就是父进程负责读数据  子进程负责写数据

```c++
int main
{    
.......
connect(sock , (Struct sockaddr*)&serv_addr , sizeof(serv_addr));
.......
if(pid == 0)
{
    write_mesg(sock ,buf);
}
else 
{
    read_mesg(sock , buf);
    close(Sock);
    return 0 ;
}
.....
}

void write_mesg(int sock ,char*buf)
{
    int str_len ;
    while(1)
    {
        fgets(buf,BUF_SZ , stdin);
        if(!strcmp(buf , "q\n"))
        {
            shutdown(sock , SHUT_WR);
            return ;
        }
    }
}
```

- #### Q:为什么可以不在子进程中关闭套接字?

**文件描述符的继承和关闭**：

- 当父进程创建子进程时，子进程继承父进程的文件描述符。尽管它们是独立的文件描述符，但它们指向相同的内核对象。
- 如果子进程没有显式地关闭套接字，而是终止了，子进程中的文件描述符将自动被操作系统关闭。这是因为操作系统会在进程退出时清理其打开的文件描述符。

**文件描述符引用计数**：

- 内核中套接字对象的生命周期由引用计数来管理。每个进程对同一个套接字对象持有一个文件描述符，引用计数就会增加。
- 当一个进程关闭文件描述符时，引用计数减少。只有当所有引用都被关闭（引用计数降到零）时，内核才会释放该套接字对象及其相关资源。
