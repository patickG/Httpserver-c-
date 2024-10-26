#pragma once
#include<iostream>
#include<unistd.h>
#include<string>
#include<vector>
#include<stdio.h>
#include<unordered_map>
#include<sstream>
#include"Util.hpp"
#include"Log.hpp"
class HttpRequest{
  public:
    std::string request_line;//请求行
    std::vector<std::string> request_header;//请求报头
    std::string blank;//请求空行
    std::string request_body;//请求正文

    //请求行的解析
    std::string method;
    std::string URI;
    std::string version;
    //请求报头的解析
    std::unordered_map<std::string,std::string> header_kv;
};

class HttpResponse
{
  public:
    std::string status_line;//状态行
    std::vector<std::string> respone_header;//相应报头
    std::string blank;//响应空行
    std::string response_body;//相应正文
};

class EndPoint
{
  private:
    int sock;
    HttpRequest http_request;
    HttpResponse http_response;
  private:
    void RecvHttpRequestLine()//读取request请求行
    {
       int sz=Untile::ReaLine(sock,http_request.request_line);
       http_request.request_line.resize(sz-1);
       log(INFO,http_request.request_line);
    }
    void RecvHttpRequestHeader()//读取request报头和空行
    {
      std::string s;
      while(true)
      {
        s.clear();
        Untile::ReaLine(sock,s);
        if(s=="\n")
        {
          http_request.blank=s;
          break;
        }
        s.resize(s.size()-1);
        http_request.request_header.push_back(s);
        log(INFO,s);
      }
    }
    void RecvHttpRequestBody()//读取request正文
    {
      Untile::ReaLine(sock,http_request.request_body);
    }

    void ParseHttpRequestLine()
    {
      auto& line=http_request.request_line;
      std::stringstream ss(line);
      ss>>http_request.method>>http_request.URI>>http_request.version;
      log(INFO,http_request.method);
      log(INFO,http_request.URI);
      log(INFO,http_request.version);
    }
    void ParseHttpRequestHeader()
    {
      for(auto str:http_request.request_header)
      {
        std::string str1,str2;
        Untile::Cutstring(str,str1,str2,": ");
        std::cout<<str1<<str2<<std::endl;
        http_request.header_kv.insert(make_pair(str1,str2));
      }
    }
  public:
    EndPoint(int _sock):sock(_sock){}
    void RecvHttpRequest()
    {
       RecvHttpRequestLine();//读取request报头和空行
      RecvHttpRequestHeader();//读取request报头和空行
    }
    void ParseHttpRequest()
    {
    ParseHttpRequestLine();

     ParseHttpRequestHeader();
    }
    void BuildHttpResponse()
    {}
    void SendHttpResponse()
    {}
    ~EndPoint()
    {}
};
class Enrance
{
  public:
    static void* HandlerRequest(void* _sock)
    {
      int sock=*(int*)_sock;
     delete _sock;
     
      EndPoint* ep=new EndPoint(sock);
      ep->RecvHttpRequest();
      ep->ParseHttpRequest();
      ep->BuildHttpResponse();
      ep->SendHttpResponse();
      delete ep;
    }
   };
