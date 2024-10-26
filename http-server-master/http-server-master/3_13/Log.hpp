#pragma once
#include<iostream>
#include<string>
#include<time.h>
//[日志级别] [时间戳] [日志信息] [错误文件名称] [错误行数]
//INFO 正常
//WARING 告警
//ERROR 出错
//FATAL 致命信息
#define INFO   1
#define WARING 2
#define ERROR  3
#define FATAL  4 

#define LOG(level,message) Log(#level,message,__FILE__,__LINE__)

void Log(std::string level,std::string message,std::string file,int line)
{
  std::cout<<"["<<level<<"]["<<time(nullptr)<<"]["<<message<<"]["<<file<<"]["<<line<<"]"<<std::endl;
}
