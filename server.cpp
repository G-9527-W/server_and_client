#include <iostream>
#include<arpa/inet.h>
#include <string>
#include <unistd.h>
#include <cstring>
#include <thread>
using namespace std;
void client_work(int c_fd,int c_port,string c_ip)
{
    std::cout<<"客户端:"<<c_ip<<"端口:"<<c_port<<"已接入"<<endl;
    char buff[1024];
   
    while(true)
    {
        memset(buff,0,sizeof(buff));
        int len=recv(c_fd,buff,sizeof(buff),0);
        if(len==0)
        {
            cout<<"客户端:"<<c_ip<<"已断开"<<endl;
            close(c_fd);
            break;
        }
        if(len<0)
        {
            cerr<<"客户端:"<<c_ip<<" 接入失败"<<endl;
            close(c_fd);
            return ;
        }
        string client_s(buff,len);
        cout<<"客户端:"<<c_ip<<"传入"<<client_s<<endl;
        int ret_s=send(c_fd,buff,len,0);
        if(ret_s<0)
        {
            cerr<<"客户端:"<<c_ip<<" 传回失败"<<endl;
            close(c_fd);
            return ;
        }
     
    }
}
int main()
{
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        cerr<<"socket is wrong";
        close(fd);
        return -1;
    }
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(8898);
    saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    int ret=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(ret<0)
    {
        cerr<<"bind is wrong";
        close(fd);
        return -1;
    }
    ret=listen(fd,128);
    if(ret<0)
    {
        cerr<<"listen is wrong";
        return -1;
    }
  

    while(true)
    {
        struct sockaddr_in caddr;
        socklen_t size_len=sizeof(caddr);
        int cfd=accept(fd,(struct sockaddr*)&caddr,&size_len);
        if(cfd<0)
       {
           cerr<<"accept is wrong";
           return -1;

        }
        char ip[32];
        inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip,sizeof(ip));
        int cport=ntohs(caddr.sin_port);
        thread client_thread(client_work,cfd,cport,ip);
        client_thread.detach();
     

    }
    close(fd);
    




}
