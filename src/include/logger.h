#pragma once
#include "lockqueue.h"
#include <string>
#include <thread>       // std::thread 支持
#include <functional>   // Lambda 支持

enum LogLevel
{
    INFO,   //普通信息
    ERROR,  //错误信息
};

//Mprpc框架提供的日志系统
class Logger
{
public:
    //获取日志的单例
    static Logger& GetInstance();

    //设置日志级别
    void SetLogLevel(LogLevel level);

    //写日志
    void Log(std::string msg);

private:
    int m_loglevel;                     //记录日志级别
    LockQueue<std::string> m_lckQue;    //日志缓冲队列

private:
    //设置成单例模式
    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};

//定义宏
// #define LOG_INFO(logmsgformat, ...) \
//     do \
//     {  \
//         Logger &logger = Logger::GetInstance(); \
//         logger.SetLogLevel(INFO);
//         char c[1024] = {0}; \
//         snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
//         logger.Log(c);
//     } while(0);

// #define LOG_ERR(logmsgformat, ...) \
//     do \
//     {  \
//         Logger &logger = Logger::GetInstance(); \
//         logger.SetLogLevel(ERROR);
//         char c[1024] = {0}; \
//         snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
//         logger.Log(c);
//     } while(0);


// 定义INFO级别的日志宏
#define LOG_INFO(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char c[1024] = {0}; \
        int written = snprintf(c, sizeof(c), logmsgformat, ##__VA_ARGS__); \
        if (written < 0 || written >= (int)sizeof(c)) { \
            fprintf(stderr, "Log message formatting error or truncation occurred\n"); \
            break; \
        } \
        logger.Log(c); \
    } while(0); 

#define LOG_ERR(logmsgformat, ...) \
    do \
    {  \
        Logger &logger = Logger::GetInstance(); \
        logger.SetLogLevel(ERROR); \
        char c[1024] = {0}; \
        int written = snprintf(c, sizeof(c), logmsgformat, ##__VA_ARGS__); \
        if (written < 0 || written >= (int)sizeof(c)) { \
            fprintf(stderr, "Log message formatting error or truncation occurred\n"); \
            break; \
        } \
        logger.Log(c); \
    } while(0); 
    