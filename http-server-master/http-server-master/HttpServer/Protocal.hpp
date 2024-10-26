#pragma once
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
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
#define BAD_REQUEST 400 
#define NOT_FOUND 404
#define SERVER_ERROR 500
#define WWW_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.1"
#define PAGE_404 "wwwroot/404.html"
#define PAGE_400 "wwwroot/400.html"
#define PAGE_500 "wwwroot/500.html"
#define LINE_END "\r\n"

//将相应的后缀转换成相应的文件类型。
//如：./html -> text/html
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
    case 400:
      description="BADREQUEST";
      break;
    case 404:
      description="NOTFOUND";
      break;
    case 500:
      description="SERVERERROR";
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
    size_t content_length;//请求正文的大小
    //请求行的解析
    std::string path;//请求路径
    std::string parameter;//参数

    bool cgi;//CGI处理的标识符
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

    int status_code;//响应的状态码
    int fd;//打开的文件描述符
    int size;//打开的文件大小
    std::string suffix;//访问资源的后缀
  public:
    HttpResponse():status_code(OK){
    }
};

class EndPoint
{
  private:
    int sock;
    HttpRequest http_request;
    HttpResponse http_response;
    bool stop;
  private:
    EndPoint():stop(false){}
    

    //读取请求
    bool RecvHttpRequestLine()//读取request请求行
    {
       int sz=Untile::ReaLine(sock,http_request.request_line);
       if(sz>0)
       {
          http_request.request_line.resize(sz-1);
          LOG(INFO,http_request.request_line);
       }
       else{
         stop=true;
       }
       return stop;
    }

    bool RecvHttpRequestHeader()//读取request报头和空行
    {
      std::string s;
      while(true)
      {
        s.clear();
        if(Untile::ReaLine(sock,s)<=0)
        {
          stop=true;
          break;
        }
        if(s=="\n")
        {
          http_request.blank=s;
          break;
        }
        s.resize(s.size()-1);
        http_request.request_header.push_back(s);
      }
      return stop;
    }


    //解析请求
    void ParseHttpRequestLine()//解析请求行
    {
      auto& line=http_request.request_line;
      std::stringstream ss(line);
      ss>>http_request.method>>http_request.URI>>http_request.version;
      auto& method=http_request.method;
      std::transform(method.begin(),method.end(),method.begin(),::toupper);
    }


    void ParseURI()//解析URI
    {
      auto& code=http_response.status_code;
      auto& method=http_request.method;
      if(method!="GET"&&method!="POST")//如果method错误，那么我们不需要进行拆
      {
        code=BAD_REQUEST;
        LOG(WARING,"method error");
      }
      else if(method=="GET")//如果是GET方法，那么我们需要判断URI中是否带参数
      {
        auto& uri=http_request.URI;
        auto iter=uri.find('?');
        if(iter!=std::string::npos)
        {
          Untile::Cutstring(uri,http_request.path,http_request.parameter,"?");
        }
        else 
        {
          http_request.path=uri;
        } 
      }
      else if(method=="POST")//如果是POST方法,那么URI中只包含路径
      {
        http_request.path=http_request.URI;
        http_request.cgi=true;
      }
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
            stop=true;
            //读取失败
            break;
          }
        }
      }
    }

    int CgiProcess()
    {
      int code=OK;
      auto& method=http_request.method;
      auto& body=http_request.request_body;
      auto& parameter=http_request.parameter;
      auto& path=http_request.path;
      pid_t pid;
      int input[2];//对于父进程来说，是进行输入
      int output[2];//对于父进程来说，是进行输出
      if(pipe(input)<0)
      {
        return SERVER_ERROR;
      }
      if(pipe(output)<0)
      {
        return SERVER_ERROR;
      }

      pid=fork();
      if(pid==0)
      {
        //子进程
        close(input[1]);
        close(output[0]);
        dup2(input[0],0);
        dup2(output[1],1);
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
          putenv((char*)parameter_env.c_str());
       }
        else if(method=="POST"){
          std::string length_env="Content-Length=";
          length_env+=std::to_string(http_request.content_length);
          std::cerr<<length_env<<std::endl;
          putenv((char*)length_env.c_str());
        }
       
        execl(path.c_str(),path.c_str(),nullptr);//execl会替换掉所有的数据和代码
        std::cout<<"execl error"<<std::endl;
        exit(1);
      }
      else if(pid>0)
      {
        //父进程
        close(input[0]);
        close(output[1]);
        int sum=0;
        int size=0;
        auto start=body.c_str();
        if(method=="POST")//如果是POST方法，通过管道将数据输入给子进程
        {
          while((size=write(input[1],start+sum,body.size()-sum))>0)
          {
             sum+=size; 
          }
        }

        char ch;
        while(read(output[0],&ch,1)>0)
        {
          http_response.response_body.push_back(ch);
        }
        std::cout<<"body:"<<http_response.response_body<<std::endl;
        int status=0;
        int ret=waitpid(pid,&status,0);
        if(ret==pid)
        {
           if(WIFEXITED(status))
           {
             if(WIFSIGNALED(status)!=0)
             {
               code=SERVER_ERROR;
             }
           }
           else{
             code=SERVER_ERROR;
           }
        }
        std::cout<<"code"<<code<<std::endl;
        close(output[0]);
        close(input[1]);
      }
      else 
      {
        //创建失败
        code=404;
      }
      return code;
    }


    int NotCgiProcess()
    {
      http_response.fd=open(http_request.path.c_str(),O_RDONLY);
      if(http_response.fd>0)
      {         
        return OK;
      }
      return SERVER_ERROR; 
    }

  public:
    EndPoint(int _sock):sock(_sock){}
   bool Stop()
   {
     return stop;
   }
    void RecvHttpRequest()
    {
      //读取请求行和请求报头，如果请求行读取错误，设置相应的错误码
       if(!RecvHttpRequestLine()&&!RecvHttpRequestHeader())
       {
         ParseHttpRequestLine();//解析请求行
         ParseURI();//解析URI
         ParseHttpRequestHeader();//解析报头
         RecvHttpRequestBody();//读取正文
       }
    }

    void BuildHttpResponse()//构建响应
    {
      std::string path;
      auto& code=http_response.status_code;
      if(code==BAD_REQUEST)
      {
        goto END;
      }
         size_t pos;
        //在路径的最前面添加wwwroot/
        
        path=http_request.path;
        http_request.path=WWW_ROOT;
        http_request.path+=path;//wwwroot/a/b/c
        if(http_request.path[http_request.path.size()-1]=='/')
        {
          http_request.path+=HOME_PAGE;//wwwroot/index.html
        }
        std::cout<<"path:"<<http_request.path<<std::endl;
 
        struct stat st;
        if(stat(http_request.path.c_str(),&st)==0)//判断该资源是否存在
        {
           if(S_ISDIR(st.st_mode))//判断是否为一个目录
           {
             //wwwroot/dir1 -> wwwroot/dir1/index.html
             //后缀都为.html
             http_request.path+="/";
             http_request.path+=HOME_PAGE;
             stat(http_request.path.c_str(),&st);
             http_response.suffix=".html";
           }
           //判断该资源是否为可执行文件
           else if(st.st_mode&S_IXGRP||st.st_mode&S_IXOTH||st.st_mode&S_IXUSR)
           {
             http_request.cgi=true;//标识cgi处理
           }
           else{
             //do nothing
           }
        }
        else{
          //说明该文件不存在
          code=NOT_FOUND;
          std::cout<<"文件不存在:"<<path<<std::endl;
          goto END;
        }   
      //设置后缀
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
      if(http_request.cgi)
      {
        //进行cgi处理
        std::cout<<"cgi begin..."<<std::endl;
        code=CgiProcess();//将cgi处理完成的数据放在响应正文中
      }
      else{
        std::cout<<"not cgi begining"<<std::endl;
        code=NotCgiProcess();//打开文本文件，后续构建http响应
      }
END:
      BuildResponseHelper();
      return ;
}

    void BuildResponseHelper()
    {
      auto& status_line=http_response.status_line;
      auto& code=http_response.status_code;
      //构建响应行
      status_line=HTTP_VERSION;
      status_line+=" ";
      status_line+=std::to_string(code);
      status_line+=" ";
      status_line+=CodeAnaly(code);
      status_line+=LINE_END;
      http_response.blank=LINE_END;
      switch(code)
      {
        //根据不同的状态码来确定处理方式
        case 200:
          Build200header();
          break;
        case 400:
         BuildErrorHeader(PAGE_400);
         break;
        case 404:
          BuildErrorHeader(PAGE_404);
        case 500:
          BuildErrorHeader(PAGE_500);
          break;
        default:
          break;
      }
    }

    void BuildErrorHeader(std::string path)//构错误的响应报头
    {      
      http_response.fd=open(path.c_str(),O_RDONLY);
      if(http_response.fd>0)//错误页面的文件被打开  
      {
        
        std::string line;
        //构建Content-Type,所有的错误页面都是文本文件，文件类型都为text/html
        line="Content-Type: text/html";
        line+=LINE_END;
        http_response.respone_header.push_back(line);
        //构建Content-Length,stat.st_size访问文本文件的大小
        line="Content-Length: ";
        struct stat st;
        if(stat(path.c_str(),&st)==0)
        {
          http_response.size=st.st_size;
        }
        line+=std::to_string(http_response.size);
        line+=LINE_END;
        http_response.respone_header.push_back(line);
      } 
    }

    void Build200header()//构建OK的响应报头
    {
      auto& cgi=http_request.cgi;
      auto& response_header=http_response.respone_header;    
      std::string line;
      //构建Content-Type,根据访问资源的后缀来判断是什么类型的文件
      //ContentThpeTable是一个根据资源返回相对应的类型
      line="Content-Type: ";
      line+=ContentTypeTable(http_response.suffix);
      line+=LINE_END;
      //构建Content-Length
      response_header.push_back(line);
      line="Content-Length: ";
      if(cgi)//如果是cgi处理，则处理的结果在响应正文中
      {
        line+=std::to_string(http_response.response_body.size());
      }
      else//如果是返回网页，则通过sendfile文件发送给浏览器，不需要读取到正中
      {
        line+=std::to_string(http_response.size);
      }
      line+=LINE_END;
      response_header.push_back(line);
    }



    void SendHttpResponse()
    {
       auto& cgi =http_request.cgi;
       send(sock,http_response.status_line.c_str(),http_response.status_line.size(),0);
       for(auto str:http_response.respone_header)
       {
         send(sock,str.c_str(),str.size(),0);
       }
      send(sock,http_response.blank.c_str(),http_response.blank.size(),0);
       if(cgi)         
       {
         //将cgi处理结果发送到网络中
         auto& response_body=http_response.response_body;
         int size=0;
         int total=0;
         const char* start=response_body.c_str();
         while(total<response_body.size()&&(size=send(sock,start+total,response_body.size()-total,0))>0)
         {
           total+=size;
         }
       }
       else{
         //返回静态网页
        sendfile(sock,http_response.fd,0,http_response.size);
        close(http_response.fd);
       }
       LOG(INFO,"SendHttpResponse");       
    }
    ~EndPoint()
    {}
};

class HandlerTask
{
  public:
    void operator()(int sock)
    {
      LOG(INFO,"HandlerTask");
      HandlerRequest(sock);
    }
  private:
    void HandlerRequest(int sock)
    {
      EndPoint* ep=new EndPoint(sock);
      ep->RecvHttpRequest();
      if(!ep->Stop())
      {
       ep->BuildHttpResponse();
       ep->SendHttpResponse();
      }
      close(sock);
      delete ep;
    }
   };
