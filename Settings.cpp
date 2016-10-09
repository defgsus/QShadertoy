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


    if (auto win = qobject_cast<QMainWindow*>(w))
    {
        key = "WindowState/" + w->objectName();
        QByteArray data;
        if (contains(key))
            data = value(key).toByteArray();
        //else
        //    data = defaultMainWindowState();

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
