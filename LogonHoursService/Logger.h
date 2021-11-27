#pragma once

#include <log4cpp/Category.hh>
#include <log4cpp/CategoryStream.hh>

// See also https://github.com/msly/codemanage/blob/master/A%20useful%20log4cpp%20example.cpp

#define CODE_LOCATION __FILE__

// DEBUG < INFO < NOTICE < WARN < ERROR < CRIT < ALERT < FATAL = EMERG

#define LOG_EMERG(__this)  Logger(__this) << log4cpp::Priority::EMERG //<< CODE_LOCATION
#define LOG_ALERT(__this)  Logger(__this) << log4cpp::Priority::ALERT //<< CODE_LOCATION
#define LOG_CRIT(__this)  Logger(__this) << log4cpp::Priority::CRIT //<< CODE_LOCATION
#define LOG_ERROR(__this)  Logger(__this) << log4cpp::Priority::ERROR //<< CODE_LOCATION
#define LOG_WARN(__this)  Logger(__this) << log4cpp::Priority::WARN //<< CODE_LOCATION
#define LOG_NOTICE(__this)  Logger(__this) << log4cpp::Priority::NOTICE //<< CODE_LOCATION
#define LOG_INFO(__this)  Logger(__this) << log4cpp::Priority::INFO //<< CODE_LOCATION
#define LOG_DEBUG(__this)  Logger(__this) << log4cpp::Priority::DEBUG //<< CODE_LOCATION

template <class T>
struct LoggerTraits
{
  static const char* Category()
  {
    return typeid(T).name();
  }
};

template <class T>
log4cpp::Category& Logger()
{
  static log4cpp::Category& logger = log4cpp::Category::getInstance(LoggerTraits<T>::Category());
  return logger;
}

template <class T>
log4cpp::Category& Logger(T* this_)
{
  return Logger<T>();
}

inline log4cpp::Category& Logger(const char* func)
{
    return log4cpp::Category::getInstance(func);
}

inline log4cpp::Category& Logger(const void* p = NULL)
{
    return log4cpp::Category::getRoot();
}
