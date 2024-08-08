# Chapter15 套接字和标准IO



##  15.1 标准IO函数

标准IO函数是指在标准输入输出库中定义的用于处理输入输出操作的函数，主要用于文件操作、数据流读写等。以下是一些常见的标准IO函数，主要来自C标准库中的`stdio.h`头文件：

#### 文件操作

- `fopen`：打开文件
- `freopen`：重新打开文件
- `fclose`：关闭文件

#### 数据读写

- `fread`：从文件中读取数据
- `fwrite`：向文件中写入数据
- `fgets`：从文件中读取一行数据
- `fputs`：向文件中写入一行数据

#### 标准输入输出流

- `stdin`：标准输入流
- `stdout`：标准输出流
- `stderr`：标准错误流

<font size =4>           **... ....还有很多**</font>



##  15.2 标准IO 的缓冲机制

- **全缓冲**：适用于大块数据的读写操作。数据先存储在缓冲区，缓冲区满后才进行实际的读写操作。
- **行缓冲**：适用于需要逐行处理数据的应用，如文本处理。每次读写一行数据，缓冲区满或者遇到换行符时进行实际的读写操作。
- **无缓冲**：适用于实时性要求高的应用，每次读写操作都立即进行，不使用缓冲区。

通过选择合适的缓冲模式和合理设置缓冲区大小，可以在不同的应用场景中显著提升系统性能



##   15.3 标准IO函数的优势

**文件读写操作频繁的应用**：

- 当程序频繁进行小块数据的读写操作时，标准IO的缓冲机制可以将这些小块数据合并成大块数据进行处理，从而减少系统调用的次数。例如，日志记录系统频繁写入日志文件，使用缓冲机制可以显著减少写入操作的开销。

**网络通信**：

- 在网络通信中，数据的发送和接收通常涉及到较高的延迟。标准IO的缓冲机制可以将多个小的数据包合并成一个大包发送，减少网络请求的次数，从而提高传输效率。比如，在HTTP服务器或客户端中，通过缓冲机制可以优化数据的传输。

**数据库系统**：

- 数据库系统经常需要频繁地读写磁盘文件，标准IO的缓冲机制可以将这些读写操作进行优化，提高数据库的访问速度。缓冲区可以缓存部分数据，减少磁盘I/O操作的次数。

**大文件处理**：

- 处理大文件时，标准IO的缓冲机制可以通过一次读取或写入较大块数据来提高效率，避免频繁的小块数据读写操作。例如，在视频处理或数据备份程序中，使用缓冲机制可以加快文件的读写速度。

**流式数据处理**：

- 在流式数据处理应用中，例如实时视频流或音频流的处理，缓冲机制可以帮助平滑数据流，提高数据处理的稳定性和效率。缓冲区可以积累一定量的数据后再进行处理，避免处理器因为等待数据而空闲。



在TCP/IP网络编程一书中,通过复制300M大小文件运行效率这一事件来对比系统IO(无缓冲区)和标准IO的运行效率,结果发现<font color=red>**标准IO的效率远远高于系统IO**</font>!!!



##    15.4 标准IO函数的劣势

- <font size=4>**不容易进行双向通信**</font>
- <font size=4>**有时可能频繁调用fflush函数**</font>

- <font size=4>**需要FILE结构体指针形式返回文件描述符**</font>



##    15.5 使用标准IO函数部分示例



###      15.5.1 fdopen函数转换为FILE结构体指针

创建套接字返回文件描述符 , 若要使用标准I/O函数需要将文件描述符转化为FILE结构体指针

```c
#include<stdio.h>
FILE* fdopen(int fildes, const char *mode);
//fildes -->文件描述符
//mode  -->"w" , "r"的模式
```



###      15.5.2 fileno 函数转换为文件描述符

```c
#include<stdio.h>
int fileno(FILE* stream);
```



### 15.5.3 fflush 函数刷新输出缓冲区

```c
#include<stdio.h>
int fflush(FILE*stream);
```

用于刷新`Stream(流)`的输入缓冲区的数据,此函数强制将给定输出或更新`Stream(流)`的所有缓冲数据并写入文件,<font color=red>**但是当应用到输入流时,其行为未知**</font>

```c
   FILE *file = fopen("example1.txt", "w");

   fprintf(file, "Hello, World!\n");

   // Ensure "Hello, World!" is written to the file immediately
   fflush(file); 
```





##  15.6 基于标准IO函数的回声客户端服务器

**服务器端:**

```c
#include<stdio.h>

struct FILE *fpread , *fpwrite ;
socklen_t clnt_adr_sz  ; 
........ ........
clnt_sock = accept(serv_sock ,(struct sockaddr*)&clnt_addr , &(clnt_adr_sz ));
fpread = fdopen(clnt_sock ,"r" );
fpwrite = fdopen(clnt_sock , "w");

while(1)
{
    fgets(buf ,BUF_SZ , fpread);
    fputs(buf , fpwrite);
    fflush(fpwrite);
}
fclose(fpread);
fclose(fpwrite);
........ ........

```

评注: 为什么要在循环内末尾调用`fflush(fpwrite)`是为了确保<font color = blue>**buf内的数据立马被写入套接字缓冲区中**,</font>并传输给客户端,减少延迟 没看懂可以再重新看一下`fflush()`的定义