#include<iostream>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#define PORT 8081
#define BACKLOG 5  
class TcpServer
{
  private:
    int port;
    int listen_sock;
    static TcpServer* tcp_svr;
  private:
    TcpServer(int _port):port(_port),listen_sock(-1)
    {}
  public:
    static TcpServer* GetTcpServer(int _port)
    {
      static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
      if(tcp_svr==nullptr)
      {
        pthread_mutex_lock(&lock); 
        if(tcp_svr==nullptr)
        {
          tcp_svr=new TcpServer(_port);
          tcp_svr->InitServer();
        }
        pthread_mutex_unlock(&lock);
        return tcp_svr;
      }
    }

    void InitServer()
    {
      Socket();
      Bind();
      Listen();
    }

   void Socket()
   {
     listen_sock=socket(AF_INET,SOCK_STREAM,0);
     if(listen_sock<0)
     {
       exit(1);
     }
   }

   void Bind()
   {
     struct sockaddr_in local;
     local.sin_family=AF_INET;
     local.sin_port=htons(port);
     local.sin_addr.s_addr=0;
     if(bind(listen_sock,(struct sockaddr*)&local,sizeof(local))<0)
     {
       exit(2);
     }
   }

   void Listen()
   {
     if(listen(listen_sock,BACKLOG))
     {
       exit(3);
     }

   }
   ~TcpServer()
   {}

};
TcpServer* TcpServer::tcp_svr=nullptr;
