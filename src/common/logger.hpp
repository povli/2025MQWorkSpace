#pragma once
#include <muduo/base/Logging.h>


#define LOG_WARNING  LOG_WARN   // 等价于 “警告” 日志
#define LOG_REQUEST  LOG_INFO   // 业务请求日志，用 INFO 级别打印


#define WARNING      WARNING    // 占位，不改写
#define REQUEST      REQUEST

// 宏展开：LOG(WARNING) -> LOG_WARNING  -> 最终映射到 LOG_WARN
#define LOG(level)   LOG_##level
