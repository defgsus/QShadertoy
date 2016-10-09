/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QWidget>

class LogView : public QWidget
{
    Q_OBJECT
public:
    explicit LogView(QWidget *parent = 0);
    ~LogView();

signals:

public slots:

private:
    struct Private;
    Private* p_;
};

#endif // LOGVIEW_H
