#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <limits>
using namespace std;
template<typename T>
void getline_s(T&t)
{
    if(cin.peek()=='\n')
    {
        cin.ignore(numeric_limits<streamsize>::max(),'\n');
    }
    getline(cin,t);
}
int main()
{
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        cerr<<" socket is wrong";
        return -1;
    }
    sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(8898);
    inet_pton(AF_INET,"172.31.152.196",&saddr.sin_addr.s_addr);
    int ret=connect(fd,(sockaddr*)&saddr,sizeof(saddr));
    if(ret==-1)
    {
        cerr<<"connect is wrong";
        close(fd);
        return -1;

    }
    while(true)
    {
       string buff_s;
       cout<<"请输入你想要向服务器传递的信息:"<<endl;
       cout<<"输入“quit”退出"<<endl;

       getline_s(buff_s);
       if(buff_s=="quit")
       {
        break;
       }
       if(buff_s.size()==0)
       {
        cout<<"不可以输入空"<<endl;
        continue;
       }
       int ret_s=send(fd,buff_s.data(),buff_s.size(),0);
       if(ret_s<0)
       {
        cerr<<"send is wrong";
        close(fd);
        return -1;
       }
       char buff_c[1024];
       int len=recv(fd,buff_c,sizeof(buff_c),0);
       if(len<1)
       {
        cerr<<"服务器断开"<<endl;
        close(fd);
        return -1;
       }
       string server_s(buff_c,len);
       cout<<"服务器返回："<<server_s<<endl;
       
    }
    close(fd);
}