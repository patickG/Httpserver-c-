#include"TcpServer.hpp"


int main(int argc,char* argv[])
{
  if(argc!=2)
  {
    std::cout<<"Usage: ./TcpServer port"<<std::endl;
  }

  int port=atoi(argv[1]);
  TcpServer* tcp=TcpServer::GetTcpServer(port);
  for(;;)
  {}
}
