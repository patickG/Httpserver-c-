#include<iostream>
#include<stdlib.h>
#include<unistd.h>
#include<string>

//取出参数
bool GetParameter(std::string& parameter)
{
   std::string method=getenv("METHOD");
   if(method=="GET")//Get方法，参数在环境变量中
   {
     parameter=getenv("PARAMETER");
   }
   else if(method=="POST")//Post方法，参数在管道中
   {
    char ch;
    int length=atoi(getenv("Content-Length"));
    while(length--)
    {
      read(0,&ch,1);
      parameter.push_back(ch);
    }
   }
   else{
     return false;
   }
   return true; 
}
//分解参数
void CutString(std::string str,std::string& strsub1,std::string& strsub2,std::string sep)
{
  size_t pos=str.find(sep);
  if(pos!=std::string::npos)
  {
    strsub1=str.substr(0,pos);
    strsub2=str.substr(pos+sep.size());
  }
}
int main()
{
  std::string parameter;  
  GetParameter(parameter);

  std::string strsub1,strsub2;//a=100 b=200
  CutString(parameter,strsub1,strsub2,"&");
  std::string name1;
  std::string value1;
  CutString(strsub1,name1,value1,"=");
  std::string name2;
  std::string value2;

  CutString(strsub2,name2,value2,"=");
  std::cout<<"<h1>";
  std::cout<<value1<<"+"<<value2<<"="<<atoi(value1.c_str())+atoi(value2.c_str());
  std::cout<<"</h1>";

  std::cout<<"<h1>";
  std::cout<<value1<<"-"<<value2<<"="<<atoi(value1.c_str())-atoi(value2.c_str());
  std::cout<<"</h1>";

  std::cout<<"<h1>";
  std::cout<<value1<<"*"<<value2<<"="<<atoi(value1.c_str())*atoi(value2.c_str());
  std::cout<<"</h1>";

  std::cout<<"<h1>";
  std::cout<<value1<<"%"<<value2<<"="<< atoi(value1.c_str())%  atoi(value2.c_str());
  std::cout<<"</h1>";
   return 0;
}

















