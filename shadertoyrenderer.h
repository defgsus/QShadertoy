/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef SHADERTOYRENDERER_H
#define SHADERTOYRENDERER_H

#include <QObject>

class ShadertoyRenderer : public QObject
{
    Q_OBJECT
public:
    explicit ShadertoyRenderer(QObject *parent = 0);

signals:

public slots:
};

#endif // SHADERTOYRENDERER_H
