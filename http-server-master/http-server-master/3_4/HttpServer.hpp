#pragma once
#include<iostream>
#include<cstdio>
#include"TcpServer.hpp"
#include"Protocal.hpp"
#include<pthread.h>
#define PORT 8081
class HttpServer
{
  private:
    int port;
    TcpServer* tcp_server;
  public:
    HttpServer(int _port=PORT):port(_port)
    {}
    void InitServer()
    {
      tcp_server=TcpServer::GetTcpServer(port);
      std::cout<<"................."<<std::endl;
    }

    void Loop()
    {
      struct sockaddr_in peer;
      socklen_t sz=0;
      int listen_sock=tcp_server->Sock();
      std::cout<<listen_sock<<std::endl;
      while(true)
      {
        
          int sock=accept(listen_sock,(struct sockaddr*)&peer,NULL);
          std::cerr<<"accept success"<<std::endl;
          if(sock<0)
          {
           continue;
          }
          int * newsock=new int(sock);
       pthread_t tid;
       pthread_create(&tid,nullptr,Enrance::HandlerRequest,newsock);
       pthread_detach(tid);
       std::cout<<"hello world"<<std::endl;
      }
      
    }

    ~HttpServer(){}
};
