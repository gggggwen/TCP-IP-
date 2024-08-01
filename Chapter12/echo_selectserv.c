#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<string.h>

void error(const char *mesg);

int main(int argc , char* argv[])
{
    int serv_sock ,clnt_sock ;
    int max_fd , fd_num ;
    fd_set reads ,cp_reads ;
    struct timeval timeout; //每次循环都要重设
    struct sockaddr_in serv_addr ,clnt_addr ; 
    socklen_t adr_sz;
    int str_len ; char buf[30];
    int i ;
    
    if(argc!=2)
    {
        printf("usage : %s <prot>\n",argv[0]);
        exit(1);
    }
    
    serv_sock = socket(AF_INET,SOCK_STREAM,0);
    memset(&serv_addr, 0 ,sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_family = AF_INET;

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1) error("bind error\n");
    if(listen(serv_sock ,5)==-1) error("listen error \n");

    FD_ZERO(&reads);
    FD_SET(serv_sock,&reads); 
    max_fd = serv_sock;

    while(1)
    {
        timeout.tv_sec = 3;
        timeout.tv_usec = 0 ;
        cp_reads = reads  ; //每次轮询前需要进行重新的拷贝
        
        fd_num = select(max_fd+1 , &cp_reads,NULL , NULL , &timeout);
        if(fd_num==-1) error("selec error\n");
        else if(fd_num ==0) continue ; //在该时间戳没有任何读事件发生

        for(i = 0 ; i<max_fd+1 ; i++)
        {
           if(FD_ISSET(i , &cp_reads))
           {
             if(FD_ISSET(serv_sock , &cp_reads)) //如果说收到了链接请求
           {
               adr_sz = sizeof(clnt_addr);
               clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_addr ,&adr_sz );
               if(clnt_sock ==-1) error("accept error\n");
               FD_SET(clnt_sock ,&reads);
               if(max_fd< clnt_sock) max_fd = clnt_sock;
               printf("connected %d.....\n",clnt_sock);
           }
           else //如果说收到了客户端传来的信息 
           {
               str_len = read(i,buf,sizeof(buf));
               if(str_len == 0 )
               {
                  //说明是客户端发出断开请求
                  FD_CLR(i , &reads);
                  printf("close %d......\n",i);
                  close(i);
               }
               else
               {
                  write(i , buf , str_len);     
               }
           }
           }

        }
        
    }
    close (serv_sock);
    return  0 ;
}

