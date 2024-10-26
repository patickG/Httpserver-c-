//工具类
//
#include<iostream>
#include<string>
#include<sys/socket.h>
#include<sys/types.h>
class Untile
{
  public:
   // 
    static int ReaLine(int sock,std::string& out)
    {
      char ch=0;
      int sz=0;
      while(ch!='\n')
      {
        sz=recv(sock,&ch,1,0);
        if(sz>0)
        {
          if(ch=='\r')
          {
            //窥探下一个字符
            recv(sock,&ch,1,MSG_PEEK);
            if(ch=='\n')
            {
              recv(sock,&ch,1,0);//将下一个字符
            }
            else 
            {
              ch='\n';
            }
          }
          //普通字符
          //\n
          out.push_back(ch);
        }
        else if(sz==0)
        {
          //对端关闭
          return 0;
        }
        else{
          //读取错误
          return -1;
        }
      }
      return out.size();
    }

  static void Cutstring(std::string target,std::string& out1,std::string& out2,std::string symbol)
      {
         int index=target.find(symbol);//寻找target字符串symbol符号的位置
         int length =symbol.size();
       if(index!=std::string::npos)//
         {                                                                                                                          //string=Content-Length: 18;
           //symbol=": "
           //out1=Content-Length
           //out2=18
           out1=target.substr(0,index);//在symbol符号之前子字符串给out1
           out2=target.substr(index+length);//在symbol符号之后的子符串给out2
         }
      }
};
