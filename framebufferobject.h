/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/30/2016</p>
*/

#ifndef FRAMEBUFFEROBJECT_H
#define FRAMEBUFFEROBJECT_H

#include <QSize>

class QOpenGLContext;

class FramebufferObject
{
public:
    FramebufferObject(QOpenGLContext*);

    const QSize& size() const { return p_size_; }

    bool create(const QSize& s);
    void release();

    /** Returns handle of current readable output texture */
    int texture() const;

    /** Swap current render target and return previous.
        FrameBufferObject must be bound! */
    int swapTexture();

    void bind();
    void unbind();

private:

    int p_createTex_();

    QOpenGLContext* p_ctx_;
    QSize p_size_;
    int p_fbo_, p_tex1_, p_tex2_;
};

#endif // FRAMEBUFFEROBJECT_H
