/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QVariant>

class QWidget;
class QSettings;

/** Application settings wrapper */
class Settings
{
    explicit Settings();
    ~Settings();
public:

    static const QString keyAppkey;

    /** Global instance */
    static Settings& instance();

    bool contains(const QString& key) const;
    QVariant value(const QString& key, const QVariant& def = QVariant()) const;

    void restoreGeometry(QWidget*) const;

    void setValue(const QString& key, const QVariant& v);
    void storeGeometry(const QWidget*);

private:
    QSettings* p_set_;
};

#endif // SETTINGS_H
