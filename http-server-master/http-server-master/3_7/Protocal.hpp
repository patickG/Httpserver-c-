#pragma once
#include<iostream>
#include<unistd.h>
#include<string>
#include<vector>
#include<algorithm>
#include<stdio.h>
#include<unordered_map>
#include<sstream>
#include"Util.hpp"
#include"Log.hpp"

#define SEP ": "
#define OK 200
#define NOT_FOUND 404
#define WWW_ROOT "wwwroot"
#define HOME_PAGE "index.html"
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
    size_t content_length;
    //请求行的解析
    std::string path;//请求路径
    std::string parameter;//参数

    bool cgi;
  public:
   HttpRequest():content_length(0),cgi(false){}
   ~HttpRequest(){}
};

class HttpResponse
{
  public:
    std::string status_line;//状态行
    std::vector<std::string> respone_header;//相应报头
    std::string blank;//响应空行
    std::string response_body;//相应正文

    int status_code;
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
      }
    }

    bool IsNeedRecvHttpRequestBody()//判断是否需要取request正文
    {
      auto& method=http_request.method;
      if(method=="POST")
      {
        //POST方法 有可能有正文
         auto& head_kv=http_request.header_kv;
         auto iter=head_kv.find("Content-Length");
         if(iter!=head_kv.end())
         {
           //找到了
           auto& content_length=http_request.content_length;
           content_length=atoi(iter->second.c_str());
           return true;
         }        
      }
      return false;
    }

    void RecvHttpRequestBody()//读取正文
    {
      if(IsNeedRecvHttpRequestBody())
      {
        auto content_length=http_request.content_length;
        char ch;
        while(content_length)
        {
          int sz=recv(sock,&ch,1,0);
          if(sz>0)
          {
            //读取成功
            http_request.request_body.push_back(ch);
            content_length--;
          }
          else{
            //读取失败
            break;
          }
        }
      }
    }

    void ParseHttpRequestLine()
    {
      auto& line=http_request.request_line;
      std::stringstream ss(line);
      ss>>http_request.method>>http_request.URI>>http_request.version;
      auto& method=http_request.method;
      std::transform(method.begin(),method.end(),method.begin(),::toupper);
    }

    void ParseHttpRequestHeader()//解析报头
    {
      for(auto str:http_request.request_header)
      {
        std::string str1,str2;
        Untile::Cutstring(str,str1,str2,SEP);
        http_request.header_kv.insert(make_pair(str1,str2));
      }
    }
  public:
    EndPoint(int _sock):sock(_sock){}
    void RecvHttpRequest()
    {
       RecvHttpRequestLine();//读取request报头和空行
       RecvHttpRequestHeader();//读取request报头和空行
       ParseHttpRequestLine();//解析请求行
       ParseHttpRequestHeader();//解析报头
       RecvHttpRequestBody();//读取正文
    }
    void BuildHttpResponse()//构建响应
    {
      auto& code=http_response.status_code;
      auto& method=http_request.method;
      if(method!="GET"&&method!="POST")
      {
        code=NOT_FOUND;
        log(WARING,"mthod error");
      }
      else if(method=="GET")
      {
        auto& uri=http_request.URI;
        auto iter=uri.find('?');
        if(iter!=std::string::npos)
        {
          Untile::Cutstring(uri,http_request.path,http_request.parameter,"?");
          log(INFO,http_request.path);
          log(INFO,http_request.parameter);
        }
        else 
        {
          http_request.path=uri;
        }
        auto path=http_request.path;
        http_request.path=WWW_ROOT;
        http_request.path+=path;
        if(http_request.path[http_request.path.size()-1]=='/')
        {
          http_request.path+=HOME_PAGE;
        }
        std::cout<<http_request.path<<std::endl;

        }
      }
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
      ep->BuildHttpResponse();
      ep->SendHttpResponse();
      delete ep;
    }
   };
