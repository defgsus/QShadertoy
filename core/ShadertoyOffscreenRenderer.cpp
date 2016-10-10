/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>

#include "ShadertoyOffscreenRenderer.h"
#include "ShadertoyShader.h"
#include "ShadertoyRenderer.h"
#include "FramebufferObject.h"
#include "log.h"

struct ShadertoyOffscreenRenderer::Private
{
    Private(ShadertoyOffscreenRenderer* p)
        : p         (p)
        , surface   (nullptr)
        , context   (nullptr)
        , fbo       (nullptr)
        , renderer  (nullptr)
        , shaderChanged (false)
    { }

    ~Private()
    {
        delete renderer;
        if (fbo)
            fbo->release();
        delete fbo;
        delete context;
        delete surface;
    }

    bool init(const QSize& res);
    bool makeCurrent();
    bool render(const QSize& res);
    QImage downloadQImage();

    ShadertoyOffscreenRenderer* p;
    ShadertoyShader shader;
    QOffscreenSurface* surface;
    QOpenGLContext* context;
    FramebufferObject* fbo;
    ShadertoyRenderer* renderer;
    bool shaderChanged;
};

ShadertoyOffscreenRenderer::ShadertoyOffscreenRenderer(QObject *parent)
    : QObject   (parent)
    , p_        (new Private(this))
{
}

ShadertoyOffscreenRenderer::~ShadertoyOffscreenRenderer()
{
    delete p_;
}


void ShadertoyOffscreenRenderer::setShader(const ShadertoyShader& s)
{
    p_->shader = s;
    p_->shaderChanged = true;
}

QImage ShadertoyOffscreenRenderer::renderToImage(const QSize& s)
{
    if (!p_->render(s))
        return QImage();

    return p_->downloadQImage();
}

bool ShadertoyOffscreenRenderer::Private::init(const QSize& res)
{
    if (!surface)
    {
        surface = new QOffscreenSurface(nullptr);

        QSurfaceFormat fmt;
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
        surface->setFormat(fmt);
        surface->create();
        if (!surface->isValid())
        {
            ST_ERROR("Offscreen surface is not created");
            return false;
        }
    }

    if (!context)
    {
        context = new QOpenGLContext();
        context->setFormat(surface->format());
        if (!context->create())
        {
            ST_ERROR("Offscreen context not created");
            return false;
        }
    }

    if (!makeCurrent())
        return false;

    if (!fbo)
    {
        fbo = new FramebufferObject(context);
    }

    if (fbo->size() != res)
    {
        if (!fbo->create(res))
        {
            ST_ERROR("Fbo for offscreen context not created");
            return false;
        }
    }

    if (!renderer)
    {
        renderer = new ShadertoyRenderer(context, surface, nullptr);
        renderer->setAsyncLoading(false);
    }

    return true;
}

bool ShadertoyOffscreenRenderer::Private::makeCurrent()
{
    if (surface && context)
    {
        if (context->makeCurrent(surface))
            return true;

        ST_ERROR("Can not make offscreen context current");
        return false;
    }
    ST_ERROR("Uninitialized offscreen context in makeCurrent()");
    return false;
}


bool ShadertoyOffscreenRenderer::Private::render(const QSize& res)
{
    if (!init(res))
        return false;

    if (shaderChanged)
    {
        renderer->setShader(shader);
        shaderChanged = false;
    }

    if (!makeCurrent())
        return false;

    return renderer->render(*fbo, false);
}

QImage ShadertoyOffscreenRenderer::Private::downloadQImage()
{
    auto gl = context->functions();

    QImage img(fbo->size(), QImage::Format_RGBA8888);

    ST_CHECK_GL( glBindTexture(GL_TEXTURE_2D, fbo->texture() ) );

    ST_CHECK_GL( glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                               img.bits()) );

    return img.mirrored(false, true);
}
