/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#include <QLayout>
#include <QTextBrowser>
#include <QTimer>

#include "LogView.h"
#include "core/log.h"

struct LogView::Private
{
    Private(LogView * w)
        : widget    (w)
    { }

    void createWidgets();
    void pullMessages();

    LogView * widget;
    QTextBrowser * edit;
    QTimer * timer;
};

LogView::LogView(QWidget *parent)
    : QWidget       (parent)
    , p_            (new Private(this))
{
    p_->createWidgets();
    p_->timer = new QTimer(this);
    p_->timer->setSingleShot(false);
    connect(p_->timer, &QTimer::timeout, [this](){ p_->pullMessages(); });
    p_->timer->start(100);
}

LogView::~LogView()
{
    p_->timer->stop();
    delete p_;
}

void LogView::Private::createWidgets()
{
    auto lv = new QVBoxLayout(widget);
    lv->setMargin(0);

    edit = new QTextBrowser(widget);
    lv->addWidget(edit);

    // XXX does not work for Mac
    QFont f("Monospace");
    f.setStyleHint(QFont::Monospace);
    edit->setFont(f);
    edit->setTabStopWidth(QFontMetrics(edit->font()).width("    "));
}

void LogView::Private::pullMessages()
{
    QString msg;
    ::Log::Level level;
    while (::Log::pullMessage(&msg, &level))
    {
        switch (level)
        {
            case ::Log::L_INFO:  edit->setTextColor(Qt::gray); break;
            case ::Log::L_WARN:  edit->setTextColor(QColor(155, 50, 20)); break;
            case ::Log::L_ERROR: edit->setTextColor(Qt::red); break;
            case ::Log::L_DEBUG: edit->setTextColor(Qt::blue); break;
            case ::Log::L_STAT:  edit->setTextColor(Qt::green); break;
        }

        while (msg.endsWith("\n"))
            msg = msg.remove(msg.length()-1, 1);
        edit->append(msg);
    };
}
