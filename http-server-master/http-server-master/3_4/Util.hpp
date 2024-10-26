//工具
//
#include<iostream>
#include<string>
#include<sys/socket.h>
#include<sys/types.h>
class Untile
{
  public:
    static int ReaLine(int sock,std::string &out)
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
            if(recv(sock,&ch,1,MSG_PEEK)=='\n')
            {
              ch=recv(sock,&ch,1,0);//将下一个字符
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
};
