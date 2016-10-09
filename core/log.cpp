/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#include <list>
#include <thread>
#include <sstream>
#include <iomanip>

#include <QMutex>
#include <QMutexLocker>

#include "log.h"

namespace {

    static std::function<void(const QString&, Log::Level)> callback_ = nullptr;

    static QMutex messageMutex_;
    static std::list<std::pair<QString, Log::Level>> messageList_;

    void print_(Log::Level level, const QString& s)
    {
        {
            QMutexLocker lock(&messageMutex_);

    #if !defined(NDEBUG)
            //std::cout << ":" << s.toStdString(); std::cout.flush();
    #endif
            messageList_.push_back(std::make_pair(s, level));

        }

        if (callback_)
            callback_(s, level);
    }

} // namespace

QString Log::threadId()
{
    std::stringstream s;
    s << "[" << std::hex << std::this_thread::get_id() << "]";
    return QString::fromStdString(s.str());
}


void Log::setCallback(std::function<void(const QString&, Level)> func)
{
    callback_ = func;
}

#ifdef MCW_USE_QT
void Log::setSocket(QTcpSocket* s)
{
    socket_ = s;
}
#endif

bool Log::pullMessage(QString* s, Level* l)
{
    QMutexLocker lock(&messageMutex_);

    if (messageList_.empty())
        return false;

    *s = messageList_.front().first;
    *l = messageList_.front().second;
    messageList_.pop_front();

    return true;
}

void Log::print(const QString& s, Level level)
{
    print_(level, s);
}
