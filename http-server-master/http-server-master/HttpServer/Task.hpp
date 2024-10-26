#pragma once
#include"Protocal.hpp"
class Task
{
  private:
    int sock;
    HandlerTask hander;
  public:
    Task(){}
    Task(int _sock):sock(_sock){}

    void HanderTask()
    {
      hander(sock);
    }

    ~Task(){}
};
