#include <iostream>
#include<arpa/inet.h>
#include <string>
#include <unistd.h>
#include <cstring>
using namespace std;
int main()
{
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        cerr<<"socket is wrong";
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
        return -1;
    }
    ret=listen(fd,128);
    if(ret<0)
    {
        cerr<<"listen is wrong";
        return -1;
    }
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
    cout<<"用户："<<ip<<" "<<cport<<" "<<"已经连入服务器"<<endl;

    while(true)
    {
        char buff[1024];
        memset(buff,0,sizeof(buff));

        int len=recv(cfd,buff,sizeof(buff),0);
        if(len>0)
        {
            string client_s(buff,len);
            cout<<"client:"<<client_s<<endl;
            send(cfd,buff,len,0);

        }
        else
        {
            cout<<"client 已断开连接"<<endl;
            break;
        }
        
     

    }
    close(fd);
    close(cfd);





}
