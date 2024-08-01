#include<stdio.h>
#include<stdlib.h>
#include<sys/select.h>
#include<sys/time.h>
#define BUF_SZ 30
//设计要求:
//读取标准输入 如果有输入重新向命令行打印一遍
int main()
{
    fd_set reads , temp ;
    struct timeval timeout;
    int res ;
    char buf[BUF_SZ];
    int str_len ; 
    
    FD_ZERO(&reads);
    FD_SET(0,&reads);
    
    while(1)
    {
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        temp = reads ;
        res =  select(1,&reads ,NULL ,NULL,&timeout); //调用select函数 监测事件发生
       
        if(res == -1)
        {
            puts("select error!\n");
            continue; ;
        }
        else if(res == 0) //超时
        {
            puts("timeout!\n");
            continue;
        }
        else  //存在事件发生 ,在本段程序中意味着有标准输入中有数据输入
        {
            if(FD_ISSET(0 , &temp))
            {
                 str_len = read(0,buf , BUF_SZ);
                 buf[str_len]=0;
                 printf("mesg from console: %s\n",buf);

            }
        }
    }
    return 0 ;
}