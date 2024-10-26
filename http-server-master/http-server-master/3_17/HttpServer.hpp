#pragma once
#include<iostream>
#include<cstdio>
#include"TcpServer.hpp"
#include"Protocal.hpp"
#include<pthread.h>
#include<signal.h>
#include"Log.hpp"
#include"Task.hpp"
#include"ThreadPool.hpp"
#define PORT 8081
class HttpServer
{
  private:
    int port;
    TcpServer* tcp_server;
    ThreadPool* threadpool;
  public:
    HttpServer(int _port=PORT):port(_port)
    {}
    void InitServer()
    {
      //信号SIGPIPE需要进行忽略，如果不忽略，如果客户端关闭，则会导致服务器挂掉
      
      signal(SIGPIPE,SIG_IGN);
      tcp_server=TcpServer::GetTcpServer(port);
      threadpool=ThreadPool::GetThreadPool();
    }

    void Loop()
    {
      int listen_sock=tcp_server->Sock();
      while(true)
      {
         struct sockaddr_in peer;
         socklen_t sz=sizeof(peer);
          int sock=accept(listen_sock,(struct sockaddr*)&peer,&sz);
          LOG(INFO,"accept success");
          if(sock<0)
          {
           continue;
          LOG(INFO,"accept error");
          }
          Task t(sock);
          threadpool->PushTask(t);
      } 
    }

    ~HttpServer(){}
};
