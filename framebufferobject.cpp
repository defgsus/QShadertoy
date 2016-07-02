/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/30/2016</p>
*/

#include <QOpenGLFunctions>

#include "framebufferobject.h"
#include "log.h"


FramebufferObject::FramebufferObject(QOpenGLContext* ctx)
    : p_ctx_        (ctx)
    , p_fbo_        (-1)
    , p_tex1_       (-1)
    , p_tex2_       (-1)
{
    ST_DEBUG_CTOR("FramebufferObject()");
}

void FramebufferObject::release()
{
    auto gl = p_ctx_->functions();

    if (p_tex1_ >= 0)
    {
        GLuint t = p_tex1_;
        gl->glDeleteTextures(1, &t);
    }
    p_tex1_ = -1;

    if (p_tex2_ >= 0)
    {
        GLuint t = p_tex2_;
        gl->glDeleteTextures(1, &t);
    }
    p_tex2_ = -1;

    if (p_fbo_ >= 0)
    {
        GLuint t = p_fbo_;
        ST_CHECK_GL( gl->glDeleteFramebuffers(1, &t) );
    }
    p_fbo_ = -1;

    p_size_ = QSize();
}


bool FramebufferObject::create(const QSize &s)
{
    ST_DEBUG2("FramebufferObject::create(" << s.width()
              << ", " << s.height() << ")");

    release();
    p_size_ = s;

    auto gl = p_ctx_->functions();

    p_tex1_ = p_createTex_();

    GLuint fbo;
    ST_CHECK_GL( gl->glGenFramebuffers(1, &fbo) );
    ST_CHECK_GL( gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo) );
    p_fbo_ = fbo;

    ST_CHECK_GL( gl->glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_tex1_, 0) );

    return true;
}

int FramebufferObject::texture() const { return p_tex1_; }
int FramebufferObject::readableTexture() const { return p_tex2_; }

int FramebufferObject::swapTexture()
{
    ST_DEBUG3("FramebufferObject::swapTexture()");

    if (p_tex1_ < 0)
        return -1;
    if (p_tex2_ < 0)
        p_tex2_ = p_createTex_();
    if (p_tex2_ < 0)
        return -1;

    auto gl = p_ctx_->functions();

    ST_CHECK_GL( gl->glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_tex2_, 0) );

    std::swap(p_tex1_, p_tex2_);

    return p_tex2_;
}

int FramebufferObject::p_createTex_()
{
    auto gl = p_ctx_->functions();

    GLuint tex;
    ST_CHECK_GL( gl->glGenTextures(1, &tex) );
    ST_CHECK_GL( gl->glBindTexture(GL_TEXTURE_2D, tex) );

    ST_CHECK_GL( gl->glTexImage2D(GL_TEXTURE_2D,
        // mipmap level
        0,
        // color components
        GL_RGBA32F,
        // size
        p_size_.width(), p_size_.height(),
        // boarder
        0,
        // input format
        GL_RGBA,
        // data type
        GL_FLOAT,
        // ptr
        nullptr) );

    ST_CHECK_GL( gl->glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_MIN_FILTER, GLint(GL_LINEAR)) );
    ST_CHECK_GL( gl->glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_MAG_FILTER, GLint(GL_LINEAR)) );

    return tex;
}

void FramebufferObject::bind()
{
    auto gl = p_ctx_->functions();
    ST_CHECK_GL( gl->glBindFramebuffer(GL_FRAMEBUFFER, p_fbo_) );
}

void FramebufferObject::unbind()
{
    auto gl = p_ctx_->functions();
    ST_CHECK_GL( gl->glBindFramebuffer(GL_FRAMEBUFFER, 0) );
}

