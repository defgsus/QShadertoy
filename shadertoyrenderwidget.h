/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/3/2016</p>
*/

#ifndef SHADERTOYRENDERWIDGET_H
#define SHADERTOYRENDERWIDGET_H

#include <QOpenGLWidget>

class ShadertoyShader;

class ShadertoyRenderWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit ShadertoyRenderWidget(QWidget *parent = 0);
    ~ShadertoyRenderWidget();

signals:

public slots:

    void setShader(const ShadertoyShader&);

    void setPlaying(bool);
    void rewind();
    //void setPlaybackTime(double);

    /** Calls update() when stopped, does nothing when running */
    void rerender();

protected:

    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void timerEvent(QTimerEvent*) override;

    void initializeGL() override;
    void paintGL() override;

private:
    struct Private;
    Private* p_;
};

#endif // SHADERTOYRENDERWIDGET_H
