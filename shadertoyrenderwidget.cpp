/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/3/2016</p>
*/

#include <QPainter>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QCursor>
#include <QMouseEvent>
#include <QTime>
#include <QBasicTimer>

#include "shadertoyrenderwidget.h"
#include "shadertoyrenderer.h"
#include "shadertoyshader.h"

struct ShadertoyRenderWidget::Private
{
    Private(ShadertoyRenderWidget* p)
        : p             (p)
        , render        (nullptr)
        , hasNewShader  (true)
        , mouseKeys     (0)
        , isPlaying     (false)
    {
    }

    ShadertoyRenderWidget* p;

    ShadertoyRenderer* render;
    ShadertoyShader shader;
    bool hasNewShader;

    QPoint mousePos;
    int mouseKeys;

    bool isPlaying;
    int frame;
    QBasicTimer timer;
    QTime timeMessure;
};


ShadertoyRenderWidget::ShadertoyRenderWidget(QWidget *parent)
    : QOpenGLWidget     (parent)
    , p_                (new Private(this))
{
    setMinimumSize(640, 384);
}

ShadertoyRenderWidget::~ShadertoyRenderWidget()
{
    makeCurrent();
    delete p_->render;
    delete p_;
    doneCurrent();
}

void ShadertoyRenderWidget::setShader(const ShadertoyShader& s)
{
    p_->shader = s;
    if (p_->render)
        p_->render->setShader(p_->shader);
    else
        p_->hasNewShader = true;
    p_->frame = 0;
    p_->timeMessure.start();
    rerender();
}

void ShadertoyRenderWidget::setPlaying(bool e)
{
    if (e == p_->isPlaying)
        return;
    p_->isPlaying = e;
    if (p_->isPlaying)
    {
        p_->timer.start(1000/60, Qt::PreciseTimer, this);
        p_->timeMessure.start();
    }
    else
        p_->timer.stop();
}

void ShadertoyRenderWidget::rewind()
{
    p_->frame = 0;
    p_->timeMessure.start();
    rerender();
}

void ShadertoyRenderWidget::rerender()
{
    if (!p_->isPlaying)
        update();
}

/*
void ShadertoyRenderWidget::setPlaybackTime(double t)
{
    p_->timeOffset = p_->timeMessure.elapsed() - t;
}
*/

void ShadertoyRenderWidget::timerEvent(QTimerEvent*)
{
    update();
}

void ShadertoyRenderWidget::mousePressEvent(QMouseEvent* e)
{
    p_->mousePos.rx() = e->x();
    p_->mousePos.ry() = height() - 1 - e->y();
    p_->mouseKeys = e->buttons();
    rerender();
}

void ShadertoyRenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    p_->mousePos.rx() = e->x();
    p_->mousePos.ry() = height() - 1 - e->y();
    p_->mouseKeys = e->buttons();
    rerender();
}

void ShadertoyRenderWidget::mouseReleaseEvent(QMouseEvent*)
{
    p_->mouseKeys = 0;
    rerender();
}



void ShadertoyRenderWidget::initializeGL()
{
    auto gl = context()->functions();

    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_CULL_FACE);
    gl->glClearColor(0,0,.5,1);

}

void ShadertoyRenderWidget::paintGL()
{
    if (!p_->shader.isValid())
    {
        QPainter p(this);
        p.setBrush(QBrush(Qt::black));
        p.setPen(QPen(Qt::white));
        p.drawRect(rect());
        p.drawText(rect(), Qt::AlignCenter, tr("no shader"));
        return;
    }

    if (!p_->render)
    {
        p_->render = new ShadertoyRenderer(context(), this);
        connect(p_->render, SIGNAL(rerender()), this, SLOT(rerender()));
    }

    if (p_->hasNewShader)
        p_->render->setShader(p_->shader);
    p_->hasNewShader = false;

    p_->render->setResolution(size());
    p_->render->setGlobalTime(float(p_->timeMessure.elapsed()) / 1000.f);
    p_->render->setFrameNumber(p_->frame++);
    p_->render->setMouse(p_->mousePos, p_->mouseKeys & Qt::LeftButton,
                                       p_->mouseKeys & Qt::RightButton);
    p_->render->setDate(QDateTime::currentDateTime());

    if (!p_->render->render(rect()))
    {
        QPainter p(this);
        p.setBrush(QBrush(Qt::black));
        p.setPen(QPen(Qt::red));
        p.drawRect(rect());
        p.drawText(rect(), Qt::AlignCenter,
                   tr("shader error\n%1").arg(p_->render->errorString()));
    }
}
