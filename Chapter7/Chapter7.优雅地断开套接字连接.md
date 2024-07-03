#  Chapter7.优雅地断开套接字连接



## 7.1对于TCP套接字命名更深刻的理

"黄河之水天上来,奔流到海不复回。"说明了水流流向的唯一性。TCP套接字(流式套接字)的数据流亦是如此,有且仅能向一个方向传输。

在TCP套接字中为了能够实现双向数据交互,因此在**套接字中会生成两种流**

![int](C:\Users\32939\AppData\Roaming\Typora\typora-user-images\image-20240702173255744.png)

## 7.2shutdown函数

用于半关闭的函数

```c++

#include<sys/socket.h>

int shutdown(int sock,int howto);

howto->传递断开方式的信息:
    SHUT_RD:断开输入流
    SHUT_WR:断开输出流
    SHUT_RDWR:同时断开I/O流
```

### 

## 7.3基于半关闭的文件传输程序



<img src="C:\Users\32939\Desktop\github_repo\TCP-IP-StudyNotes\Chapter7\屏幕截图 2024-07-02 174418.png" alt="屏幕截图 2024-07-02 174418" style="zoom:67%;" />

​                             原理大致如下:服务器发送到*EOF*预示文件传输结束,随后半关闭服务器端连接(shutdown!!!!),客户端随后向服务器发送"Thank you",最后关闭连接。

###     7.3.1代码实现

​                                                                          **客户端代码**

```c++
   //......................................省略代码....................................
    FILE *fp;
    char bf[BUF_SIZE];
    fp = fopen("file_path","wb");
    while(1)
    {
        read_count = read(serv_sock,bf,sizeof(bf));
        if(read_count <BUF_SIZE)
            fwrite((void*)bf,1,read_count,fp);
            break;
        fwrite((void*)bf,1,BF_SIZE,read_count,fp);
    }
    //或者
    while((read_count = read(serv_sock,bf,sizeof(bf))!=0)
          {
              fwrite((void*)bf ,1,BF_SIZE ,read_count,fp);
          }
    write(serv_sock,"thank you!",11);      

```

​                                                                          **服务器端代码**

```c++
FILE *fp;
char bf[BUF_SIZE];
fp = fopen("file_path","wb");

while(1)
{
	read_cnt=fread((void*)buf, 1, BUF_SIZE, fp);
	if(read_cnt<BUF_SIZE)
	{
		write(clnt_sd, buf, read_cnt);
		break;
	}
	write(clnt_sd, buf, BUF_SIZE);
}
shutdown(clnt_sock,SHUT_WR);
read(clnt_sock,bf,sizeof(bf));
printf("mesg from serv %s",bf);

```

###   7.3.2读写函数:

读:

```c
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
//ptr: 指向一个内存块的指针，用于存储读取的数据。
//size: 每个数据块的大小（以字节为单位）。一般指定为1
//nmemb: 要读取的元素数量。(一般指定为字节数)
//stream: 指向 FILE 对象的指针，该对象指定了输入流。
size_t read(int fd , (void*)buf ,size_t nbytes);


```

- 注意fread函数,读取一个文件时,如果说文件过大或者buffer空间过小,文件无法一次性全部读取,fread函数会记录每次读取时的位置,当下次读取后直接从该位置开始读取(**并非再从头开始读取**)

- read fread都以EOF为结束读取,或以buffer已满为当前读取结束标志

   

写:

```c
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
ssize_t write(int fd ,(void*) bf ,size_t nbytes)
```

- **返回值说明 :** size_t 返回值返回的是实际写出到文件的 基本单元 个数 ;
