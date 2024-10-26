#pragma once
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
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
#define HTTP_VERSION "HTTP/1.1"
#define LINE_END "\r\n"

static std::string ContentTypeTable(std::string suffix)
{
  std::unordered_map<std::string,std::string> table{
    {".html","text/html"},{".txt","text/plain"},
    {".ppt","application/vnd.ms-powerpoint"},{".js","application/x-javascript"},
    {".xml","application/rss+xml"}
  };

  auto iter=table.find(suffix);
  if(iter!=table.end())
  {
    return iter->second;
  }
  return ".html";
}

static std::string CodeAnaly(int code)
{
  std::string description;
  switch(code)
  {
    case 200:
      description= "OK";
      break;
    case 404:
      description="NOTFOUND";
      break;
    default:
      break;
  }
  return description;
}

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
    int fd;
    int size;//打开的文件大小
    std::string suffix;
  public:
    HttpResponse():status_code(200){
    }
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
       LOG(INFO,http_request.request_line);
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

    int CgiProcess()
    {
      auto& method=http_request.method;
      auto& body=http_request.request_body;
      auto& parameter=http_request.parameter;
      pid_t pid;
      int input[2];//对于父进程来说，是进行输入
      int output[2];//对于父进程来说，是进行输出
      if(pipe(input)<0)
      {
        return 404;
      }
      if(pipe(output)<0)
      {
        return 404;
      }

      pid=fork();
      if(pid==0)
      {
        //子进程
        close(input[1]);
        close(output[0]);
        dup2(output[1],0);
        dup2(input[0],1);
        //环境变量不会被env给替换掉
        //将方法导入到子进程的环境变量中，让替换的子进程能够判断数据从哪里拿上来
        std::string method_env="METHOD=";
        method_env+=method;
        putenv((char*)method_env.c_str());

        if(method=="GET")
        {
          //如果是GET方法，GET方法传输的数据比较短，可以通过环境变量传输给子进程
          std::string parameter_env="PARAMETER=";
          parameter_env+=parameter;
          putenv((char*)parameter.c_str());
        }        
        execl(method.c_str(),method.c_str(),nullptr);//execl会替换掉所有的数据和代码
      }
      else if(pid>0)
      {
        //父进程
        close(input[0]);
        close(output[1]);
        int sum=0;
        int size=0;
        if(method=="POST")//如果是POST方法，通过管道将数据输入给子进程
        {
          while(size=send(input[1],body.c_str(),body.size()-sum,0)>0)
          {
             sum+=size; 
          }
        }
      }
      else 
      {
        //创建失败
        return 404;
      }
      return OK;
    }

    int NotCgiProcess()
    {
      http_response.fd=open(http_request.path.c_str(),O_RDONLY);
      if(http_response.fd>0)
      {
        http_response.status_line=HTTP_VERSION;
        http_response.status_line+=" ";
        http_response.status_line+=std::to_string(http_response.status_code);
        http_response.status_line+=" ";
        http_response.status_line+=CodeAnaly(http_response.status_code);
        http_response.status_line+=LINE_END;

        std::string response_content_length="Content-Length: ";
        response_content_length+=std::to_string(http_response.size);
        response_content_length+=LINE_END;
        http_response.respone_header.push_back(response_content_length);
         
        std::string response_constent_type="Content-Type: ";
        response_constent_type+=ContentTypeTable(http_response.suffix);
        http_response.blank=LINE_END;
        http_response.respone_header.push_back(response_constent_type);
        return OK;
      }
      return 404; 
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
      auto path=http_request.path;
      auto& code=http_response.status_code;
      auto& method=http_request.method;
      size_t pos;
      if(method!="GET"&&method!="POST")
      {
        code=NOT_FOUND;
        LOG(WARING,"mthod error");
        goto END;
      }
      else if(method=="GET")
      {
        auto& uri=http_request.URI;
        auto iter=uri.find('?');
        if(iter!=std::string::npos)
        {
          Untile::Cutstring(uri,http_request.path,http_request.parameter,"?");
          LOG(INFO,http_request.path);
          LOG(INFO,http_request.parameter);
        }
        else 
        {
          http_request.path=uri;
        } 
      }
      else if(method=="POST")
      {
        http_request.cgi=true;
      }
        //在路径的最前面添加wwwroot/
        http_request.path=WWW_ROOT;
        http_request.path+=path;//wwwroot/a/b/c
        if(http_request.path[http_request.path.size()-1]=='/')
        {
          http_request.path+=HOME_PAGE;//wwwroot/index.html
        }
        std::cout<<http_request.path<<std::endl;
 
        struct stat st;
        if(stat(http_request.path.c_str(),&st)==0)
        {
          //说明该文件是存在的
           if(S_ISDIR(st.st_mode))
           {
             //这是一个目录
             http_request.path+="/";
             http_request.path+=HOME_PAGE;
             stat(http_request.path.c_str(),&st);
             http_response.suffix=".html";
           }
           else if(st.st_mode&S_IXGRP||st.st_mode&S_IXOTH||st.st_mode&S_IXUSR)
           {
             //该文件具有可执行权限
             http_request.cgi=true;
           }
           else{
             //do nothing
           }
            http_request.path.rfind(".");
        }
        else{
          //说明该文件不存在
          goto END;
        }   
      http_response.size=st.st_size;

      pos=http_request.path.rfind(".");
      if(pos!=std::string::npos)
      {
        //找到后缀
        http_response.suffix=http_request.path.substr(pos);
      }
      else{
        //找不到后缀,默认设置为.html
        http_response.suffix=".html";
      }
      std::cout<<http_response.suffix<<std::endl;
      if(http_request.cgi)
      {
         //只要有传参就需要进行cgi处理
        code=CgiProcess();
      }
      else{
        code=NotCgiProcess();//静态网页的返回
      }
END:
      return ;
      }
    void SendHttpResponse()
    {
       send(sock,http_response.status_line.c_str(),http_response.status_line.size(),0);
       for(auto str:http_response.respone_header)
       {
         send(sock,str.c_str(),str.size(),0);
         std::cout<<str;
       }
       send(sock,http_response.blank.c_str(),http_response.blank.size(),0);
       sendfile(sock,http_response.fd,0,http_response.size);
       close(http_response.fd);
       LOG(INFO,"SendHttpResponse");       
    }
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
      close(sock);
      delete ep;
    }
   };
