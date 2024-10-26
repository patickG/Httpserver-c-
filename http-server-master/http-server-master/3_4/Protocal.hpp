#pragma once
#include<iostream>
#include<unistd.h>
class Enrance
{
  public:
    static void* HandlerRequest(void* _sock)
    {
      int sock=*(int*)_sock;
     delete _sock;
     
    std::cout<<"hello world"<<std::endl;
    
     close(sock);
     return nullptr;
    }
};
