# Chapter 16 关于I/O分流



## 16.1 对比第十章实现TCP的I/O分割

在第十章中,用多进程模型实现I/O分割 , 父进程负责将数据写入套接字 ,子进程负责读取套接字的数据

在本章节中, 我们想要借助`FILE`指针和标准IO函数实现一下优雅关闭套接字





## 16.2 证明fclose(fwrite) 会导致将整个套接字关闭

- #### 服务器端

```c
    fputs("hello clnt_sock \n",fpwrite);
    fflush(fpwrite);
    fclose(fpwrite); //半关闭
    fgets(mesg,sizeof(mesg), fpread);
    fputs(mesg,stdout);
    fclose(fpread);

```



- #### 客户端

```c
	while(1)
	{
		if(fgets(buf, sizeof(buf), readfp)==NULL) 
			break;
		fputs(buf, stdout);
		fflush(stdout);
	 }  

	fputs("FROM CLIENT: Thank you! \n", writefp);
	fflush(writefp);
	fclose(writefp); fclose(readfp);
	return 0;
}
```

<font size =4 >**说明**:</font>

服务器端先向客户端打招呼,而后客户端读取数据 并向服务器端发送`"FROM CLIENT: Thank you! \n"`

但是服务端未收到 , <font color =red>**说明 `fclose(fpwrite)`关闭了套接字**</font>

<img src="./TCP-IP-StudyNotes/phtoto/image-20240808155959029.png" alt="image-20240808155959029" style="zoom:80%;" />



## 16.3无法实现半关闭的原因

下面的图描述的是服务端代码中的两个FILE 指针、文件描述符和套接字中的关系。

[![img](https://camo.githubusercontent.com/fe7634e5182fb8d8892d110bf2218e2cd178df65cc1b671bea1478d906db14d9/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313231646138393935352e706e67)](https://camo.githubusercontent.com/fe7634e5182fb8d8892d110bf2218e2cd178df65cc1b671bea1478d906db14d9/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313231646138393935352e706e67)

从图中可以看到，两个指针都是基于同一文件描述符创建的。因此，针对于任何一个 FILE 指针调用 fclose 函数都会关闭文件描述符，如图所示：

[![img](https://camo.githubusercontent.com/0f299803e5a41f654e92d068be714816f3762dc6add7c1b562408770e1916695/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313232343035313830322e706e67)](https://camo.githubusercontent.com/0f299803e5a41f654e92d068be714816f3762dc6add7c1b562408770e1916695/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313232343035313830322e706e67)

从图中看到，销毁套接字时再也无法进行数据交换。那如何进入可以进入但是无法输出的半关闭状态呢？如下图所示：

[![img](https://camo.githubusercontent.com/81c1599a959a743c49c817516920791a9b15b7da3953aaf19f5a5923308dcba7/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313232613435633566312e706e67)](https://camo.githubusercontent.com/81c1599a959a743c49c817516920791a9b15b7da3953aaf19f5a5923308dcba7/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313232613435633566312e706e67)

只需要创建 FILE 指针前先复制文件描述符即可。复制后另外创建一个文件描述符，然后利用各自的文件描述符生成读模式的 FILE 指针和写模式的 FILE 指针。这就为半关闭创造好了环境，因为套接字和文件描述符具有如下关系：

> **销毁所有文件描述符候才能销毁套接字**

也就是说，针对写模式 FILE 指针调用 fclose 函数时，只能销毁与该 FILE 指针相关的文件描述符，无法销毁套接字，如下图：

[![img](https://camo.githubusercontent.com/3bef530b4a7e12af7857db280cd4606aee9b657d767a8ce80ebcb50efe28c0ab/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313233616437646633312e706e67)](https://camo.githubusercontent.com/3bef530b4a7e12af7857db280cd4606aee9b657d767a8ce80ebcb50efe28c0ab/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313233616437646633312e706e67)

那么调用 fclose 函数候还剩下 1 个文件描述符，因此没有销毁套接字。那此时的状态是否为半关闭状态？不是！**只是准备好了进入半关闭状态，而不是已经进入了半关闭状态。仔细观察，还剩下一个文件描述符。而该文件描述符可以<font color=blue>同时进行 I/O </font>。因此，不但没有发送 EOF ，而且仍然可以利用文件描述符进行输出**



## 16.4 dup 和dup2 复制文件描述符

与调用 fork 函数不同，调用 fork 函数将复制整个进程，此处讨论的是同一进程内完成对完成描述符的复制。如图：

[![img](https://camo.githubusercontent.com/7b941dfce2940e058266434a1d24ba2aa8b88093403385517c82ae9ff15bb12a/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313235373963343562362e706e67)](https://camo.githubusercontent.com/7b941dfce2940e058266434a1d24ba2aa8b88093403385517c82ae9ff15bb12a/68747470733a2f2f692e6c6f6c692e6e65742f323031392f30312f33302f356335313235373963343562362e706e67)

复制完成后，两个文件描述符都可以访问文件，但是编号不同。



```c
#include<unistd.h>

int dup(int fileds);
int dup2(int fildes , int fildes2 );

//fildes 需要复制的文件描述符
//fildes2 明确指定的文件描述符的整数
//成功返回复制的文件描述符 ,失败返回-1
```



只需将源代码稍微一改即可实现I/O分离

```c
fpread = fdopen(clnt_sock, "r");
fpwrite = fdopen(dup(clnt_sock),"w");
```

