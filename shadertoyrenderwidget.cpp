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
#include <QToolButton>
#include <QLabel>
#include <QLayout>
#include <QComboBox>

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
        , projectMode   (ShadertoyRenderer::P_RECT)
        , isPlaying     (false)
        , tryRender     (true)
        , pauseTime     (0.f)
        , timeOffset    (0.f)
    {
    }

    ShadertoyRenderWidget* p;

    ShadertoyRenderer* render;
    ShadertoyShader shader;
    bool hasNewShader;

    QPoint mousePos;
    int mouseKeys;
    ShadertoyRenderer::Projection projectMode;

    bool isPlaying, tryRender;
    int frame;
    QBasicTimer timer;
    QTime timeMessure;
    float pauseTime, timeOffset;

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
    p_->tryRender = true;
    rerender();
}

void ShadertoyRenderWidget::setPlaying(bool e)
{
    if (e == p_->isPlaying)
        return;
    if (!e)
        p_->pauseTime = playbackTime();

    p_->isPlaying = e;
    if (p_->isPlaying)
    {
        // XXX well, how 'Precise' can integer be?
        p_->timer.start(1000/60, Qt::PreciseTimer, this);
        p_->timeMessure.start();
        p_->timeOffset = p_->pauseTime;
    }
    else
    {
        p_->timer.stop();
    }
}

void ShadertoyRenderWidget::rewind()
{
    p_->frame = 0;
    p_->pauseTime = 0.f;
    p_->timeOffset = 0.f;
    p_->timeMessure.start();
    rerender();
}

void ShadertoyRenderWidget::rerender()
{
    if (!p_->isPlaying)
        update();
}

float ShadertoyRenderWidget::playbackTime() const
{
    if (p_->isPlaying)
        return p_->timeOffset + float(p_->timeMessure.elapsed()) / 1000.f;
    else
        return p_->pauseTime;
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
    p_->render->setProjection(p_->projectMode);
    p_->render->setGlobalTime(playbackTime());
    p_->render->setFrameNumber(p_->frame++);
    p_->render->setMouse(p_->mousePos, p_->mouseKeys & Qt::LeftButton,
                                       p_->mouseKeys & Qt::RightButton);
    p_->render->setDate(QDateTime::currentDateTime());

    bool r = false;
    if (p_->tryRender)
        r = p_->render->render(rect());

    if (!r)
    {
        QPainter p(this);
        p.setBrush(QBrush(Qt::black));
        p.setPen(QPen(Qt::red));
        p.drawRect(rect());
        p.drawText(rect(), Qt::AlignCenter,
                   tr("shader error\n%1").arg(p_->render->errorString()));
        p_->tryRender = false;
    }
}

QWidget* ShadertoyRenderWidget::createPlaybar(QWidget*parent)
{
    QWidget* container = new QWidget(parent);
    auto lh = new QHBoxLayout(container);

        auto but = new QToolButton(container);
        but->setText("|<");
        lh->addWidget(but);
        connect(but, &QToolButton::clicked, [=]()
        {
            rewind();
        });

        but = new QToolButton(container);
        but->setText(">");
        lh->addWidget(but);
        connect(but, &QToolButton::clicked, [=]()
        {
            setPlaying(true);
        });

        but = new QToolButton(container);
        but->setText("#");
        lh->addWidget(but);
        connect(but, &QToolButton::clicked, [=]()
        {
            setPlaying(false);
        });

        lh->addStretch();

        auto cb = new QComboBox(container);
        cb->addItem(tr("rectangular"), (int)ShadertoyRenderer::P_RECT);
        cb->addItem(tr("fisheye"), (int)ShadertoyRenderer::P_FISHEYE);
        cb->addItem(tr("cross-eye"), (int)ShadertoyRenderer::P_CROSS_EYE);
        lh->addWidget(cb);
        connect(cb, static_cast<void(QComboBox::*)(int)>
                        (&QComboBox::currentIndexChanged), [=]()
        {
            p_->projectMode = (ShadertoyRenderer::Projection)
                    cb->currentData().toInt();
            update();
        });

        lh->addStretch();

        auto timeLabel = new QLabel(container);
        timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        lh->addWidget(timeLabel);
        connect(this, &ShadertoyRenderWidget::frameSwapped,
                [=]()
        {
            timeLabel->setText(QString("%1").arg(playbackTime()));
        });

    return container;
}
