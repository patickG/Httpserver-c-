#include"HttpServer.hpp"
#include<stdlib.h>
//./http_server 8081
//
void Usage()
{
  std::cout<<"Usage : ./http_server port"<<std::endl;
}
int main(int argc,char* argv[])
{
  if(argc!=2)
  {
    Usage();
    return 0;
  }

  std::cout<<"******"<<std::endl;
  int port=atoi(argv[1]);
  TcpServer* hs=TcpServer::GetTcpServer(port);

  std::cout<<"******"<<std::endl;
  hs->InitServer();
  std::cout<<"111111111"<<std::endl;
  std::cout<<hs->Sock();

  std::cout<<"iiiiiii"<<std::endl;

  return 0;
}
