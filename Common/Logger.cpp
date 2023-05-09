#include "stdafx.h"
#include "Logger.h"

#include <log4cpp/DailyRollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Win32DebugAppender.hh>

#include "version.h"

#pragma comment(lib, "ws2_32.lib")                      // Needed for log4cpp if PropertyConfigurator is used

void LogMain(const char* appname)
{
    log4cpp::Category& root = log4cpp::Category::getRoot();

    ////////////////////////////////////////
    // Configure log4cpp
    ////////////////////////////////////////
    char config_path[_MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, config_path, _countof(config_path));
    strcat_s(config_path, ".log4cpp");
    try
    {
        log4cpp::PropertyConfigurator::configure(config_path);
    }
    catch (const log4cpp::ConfigureFailure& /*ex*/)
    {
        config_path[0] = 0;

        // Default configuration for log4cpp
#ifdef _DEBUG
        root.setPriority(log4cpp::Priority::DEBUG);
#else
        root.setPriority(log4cpp::Priority::INFO);
#endif

    }

    const log4cpp::AppenderSet& set = root.getAllAppenders();
    if (set.empty())            // if no appenders are specified in the config file ...
    {
        char log_path_format[MAX_PATH];
        ExpandEnvironmentStringsA("%TEMP%\\%s.log", log_path_format, _countof(log_path_format));

        char log_path[MAX_PATH];
        if (appname)
            sprintf_s(log_path, log_path_format, appname);
        else
        {
            char module_path[_MAX_PATH] = { 0 };
            GetModuleFileNameA(NULL, module_path, _countof(module_path));

            char filename[MAX_PATH] = { 0 };
            _splitpath_s(module_path, NULL, 0, NULL, 0, filename, _countof(filename), NULL, 0);
            sprintf_s(log_path, log_path_format, filename);
        }

#if 1
        if (log4cpp::Appender* const appender = new log4cpp::DailyRollingFileAppender("logfile", log_path, 14))
#else
        if (log4cpp::Appender* const appender = new log4cpp::RollingFileAppender("logfile", log_path, 10*1024*1024, 10))
//#else
        if (log4cpp::Appender* const appender = new log4cpp::FileAppender("logfile", log_path))
#endif
        {
            log4cpp::PatternLayout* const layout = new log4cpp::PatternLayout();
            // Refer to http://www.cplusplus.com/reference/ctime/strftime/ for format specification
            layout->setConversionPattern("%d{%Y.%m.%d %H:%M:%S,%l} [%p] [%t] %c: %m%n");
            appender->setLayout(layout);
            root.addAppender(appender);
        }
        if (log4cpp::Appender* const appender = new log4cpp::Win32DebugAppender("debugger"))
        {
            log4cpp::PatternLayout* const layout = new log4cpp::PatternLayout();
            // Refer to http://www.cplusplus.com/reference/ctime/strftime/ for format specification
            layout->setConversionPattern("%d{%H:%M:%S,%l} [%p] [%t] %c: %m%n");
            appender->setLayout(layout);
            root.addAppender(appender);
        }
    }

    LOG_INFO(__func__) << "========================================";
    LOG_INFO(__func__) << "Log initialized successfully";
    if (*config_path)
        LOG_INFO(__func__) << "Loaded " << config_path;

#ifndef _CONSOLE
    LOG_INFO(__func__) << "Length of command line: " << _tcslen(GetCommandLine());
#endif

    LOG_INFO(__func__) << "Version: " << VERSION_STRING;
    LOG_INFO(__func__) << "Build  : " << __DATE__ << ' ' << __TIME__;
#if defined(_MSC_FULL_VER)
    LOG_INFO(__func__) << "_MSC_FULL_VER: "
        << _MSC_FULL_VER / 10000000 << '.'
        << (_MSC_FULL_VER % 10000000) / 100000 << '.'
        << _MSC_FULL_VER % 100000;
#elif defined(_MSC_VER)
    LOG_INFO(__func__) << "_MSC_VER: "
        << _MSC_VER / 100 << '.'
        << _MSC_VER % 100;
#endif

}
