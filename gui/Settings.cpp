/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include <cassert>

#include <QSettings>
#include <QApplication>
#include <QMainWindow>
#include <QByteArray>

#include "Settings.h"

namespace { static Settings* instance_ = nullptr; }

const QString Settings::keyAppkey = "AppKey";

Settings::Settings()
{
    p_set_ = new QSettings(
                QSettings::IniFormat,
                QSettings::UserScope,
                "modular-audio-graphics", "shadertoy", qApp);
}

Settings::~Settings()
{
    delete p_set_;
    instance_ = nullptr;
}

Settings& Settings::instance()
{
    if (!instance_)
        instance_ = new Settings();
    return *instance_;
}


void Settings::storeGeometry(const QWidget* w)
{
    assert(!w->objectName().isEmpty() && "no objectName() given to widget");
    QString key = "Geometry/" + w->objectName();
    auto data = w->saveGeometry();
    setValue(key, data);

    if (auto win = qobject_cast<const QMainWindow*>(w))
    {
        key = "WindowState/" + w->objectName();
        data = win->saveState();
        setValue(key, data);
    }
}

void Settings::restoreGeometry(QWidget* w) const
{
    assert(!w->objectName().isEmpty() && "no objectName() given to widget");
    QString key = "Geometry/" + w->objectName();

    if (contains(key))
    {
        auto data = value(key).toByteArray();
        if (!data.isEmpty())
            w->restoreGeometry(data);
    }

    // in case of a QMainWindow
    if (auto win = qobject_cast<QMainWindow*>(w))
    {
        key = "WindowState/" + w->objectName();
        QByteArray data;
        if (contains(key))
            data = value(key).toByteArray();
        else
            data = p_defaultMainWindowState();

        if (!data.isEmpty())
            win->restoreState(data);
    }
}


bool Settings::contains(const QString& key) const
{
    return p_set_->contains(key);
}

QVariant Settings::value(const QString& key, const QVariant& def) const
{
    return p_set_->value(key, def);
}

void Settings::setValue(const QString& key, const QVariant& v)
{
    p_set_->setValue(key, v);
}

QByteArray Settings::p_defaultMainWindowState()
{
    return QByteArray::fromPercentEncoding(
                "%00%00%00%FF%00%00%00%00%FD%00%00%00%01%00%00%00%02%00%00%05%9E%00%00%02%D6%FC%01%00%00%00%05%FB%00%00%00%18%00R%00e%00n%00d%00e%00r%00W%00i%00d%00g%00e%00t%01%00%00%00%00%00%00%02%92%00%00%02%92%00%FF%FF%FF%FB%00%00%00%10%00I%00n%00f%00o%00V%00i%00e%00w%01%00%00%02%98%00%00%01%2C%00%00%00%8B%00%00%01%2C%FC%00%00%03%CA%00%00%01%D4%00%00%01F%00%FF%FF%FF%FA%00%00%00%00%02%00%00%00%03%FB%00%00%00%14%00S%00h%00a%00d%00e%00r%00L%00i%00s%00t%01%00%00%00%00%FF%FF%FF%FF%00%00%00%9D%00%FF%FF%FF%FB%00%00%00%10%00P%00a%00s%00s%00V%00i%00e%00w%01%00%00%00%00%FF%FF%FF%FF%00%00%017%00%FF%FF%FF%FB%00%00%00%10%00P%00l%00o%00t%00V%00i%00e%00w%01%00%00%00%00%FF%FF%FF%FF%00%00%00%5B%00%FF%FF%FF%FC%00%00%01F%00%00%02%C3%00%00%00%00%00%FF%FF%FF%FA%FF%FF%FF%FF%02%00%00%00%00%FC%00%00%04X%00%00%01F%00%00%00%00%00%FF%FF%FF%FA%FF%FF%FF%FF%02%00%00%00%00%00%00%05%9E%00%00%00%00%00%00%00%04%00%00%00%04%00%00%00%08%00%00%00%08%FC%00%00%00%00"
                );
}
