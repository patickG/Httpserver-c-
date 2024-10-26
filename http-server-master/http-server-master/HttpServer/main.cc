#include"HttpServer.hpp"
#include<stdlib.h>
#include<memory>
#include"Log.hpp"
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
  

  int port=atoi(argv[1]);
  std::shared_ptr<HttpServer> http_server(new HttpServer(port));
  HttpServer* hs=new HttpServer(port);

  hs->InitServer();
  hs->Loop();


  return 0;
}
