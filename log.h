/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#ifndef LOG_H
#define LOG_H

#include <iostream>

#include <QString>
#include <QTextStream>

/** Namespace for static functions for logging. */
struct Log
{
    enum Level
    {
        L_INFO,
        L_WARN,
        L_ERROR,
        L_DEBUG,
        L_STAT
    };

    static QString threadId();

    /** Installs a function that receives log messages.
        Previous function is overwritten.
        <b>The callback will come from the thread that is logging!</b> */
    static void setCallback(std::function<void(const QString&, Level)> func);

    /** Pulls a message from the log que.
        Returns false if no more messages are currently available.
        <b>Threadsafe</b> */
    static bool pullMessage(QString*, Level*);

    // ------- logging interface -------

    /** Print the string */
    static void print(const QString&, Level level = L_INFO);
};

#ifdef NDEBUG
    /** Implementation of macro stream logging */
    #define ST_LOG_IMPL_(level__, prefix_unused__, arg__) \
      { ::QString s__; QTextStream ts__(&s__); ts__ << arg__ << "\n"; \
        ::Log::print(s__, level__); }
#else
    #define ST_LOG_IMPL_(level__, prefix__, arg__) \
      { ::QString s__; QTextStream ts__(&s__); \
        ts__ << ::Log::threadId() << " " << arg__ << "\n"; \
        std::cout << "[" << prefix__ << "] " << s__.toStdString(); \
            std::cout.flush(); \
        ::Log::print(s__, level__); }
#endif

#define ST_INFO(arg__)          ST_LOG_IMPL_(::Log::L_INFO,  "INFO ", arg__)
#define ST_WARN(arg__)          ST_LOG_IMPL_(::Log::L_WARN,  "WARN ", arg__)
#define ST_ERROR(arg__)         ST_LOG_IMPL_(::Log::L_ERROR, "ERROR", arg__)
#define ST_LOGIC_ERROR(arg__)   ST_LOG_IMPL_(::Log::L_ERROR, "ERROR", arg__)
#define ST_DEBUG(arg__)         ST_LOG_IMPL_(::Log::L_DEBUG, "DEBUG", arg__)
#ifndef NDEBUG
#   define ST_DEBUG2(arg__)     ST_LOG_IMPL_(::Log::L_DEBUG, "DEBUG", arg__)
#   define ST_DEBUG_CTOR(arg__) ST_LOG_IMPL_(::Log::L_DEBUG, "DEBUG", arg__)
#else
#   define ST_DEBUG2(unused__) { }
#   define ST_DEBUG_CTOR(unused__) { }
#endif

#if 1 && !defined(NDEBUG)
#   define ST_DEBUG3(arg__)     ST_LOG_IMPL_(::Log::L_DEBUG, "DEBUG", arg__)
#else
#   define ST_DEBUG3(unused__) { }
#endif

#ifndef NDEBUG
#   define ST_CHECK_GL(code__) \
        { code__; GLenum err__ = gl->glGetError(); \
            if (err__ != GL_NO_ERROR) { ST_ERROR("OpenGL error " << err__ \
                                         << " for '" << #code__ << "'\n" \
                                         << __FILE__ << ":" << __LINE__); } }
#else
#   define ST_CHECK_GL(code__) { code__; }
#endif

#endif // LOG_H
