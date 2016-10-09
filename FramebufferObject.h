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

/** Simple wrapper about an OpenGL framebuffer object.
    @note Qt's QOpenGLFramebufferObject does not support
    efficient swapping of the color attachment texture
    so here's a simple replacement. */
class FramebufferObject
{
public:
    /** Context is needed for QOpenGLFunctions */
    FramebufferObject(QOpenGLContext*);

    /** Current resolution */
    const QSize& size() const { return p_size_; }

    /** Creates a new fbo with color attachment for given resolution.
        Previous fbo will be released. */
    bool create(const QSize& s);
    /** Release all OpenGL resources.
        Does nothing if nothing is created. */
    void release();

    /** Returns handle of current written-to output texture */
    int texture() const;
    /** Returns handle of previously written-to output texture */
    int readableTexture() const;

    /** Swap current render target and return previous.
        FrameBufferObject must be bound! */
    int swapTexture();

    /** Bind the fbo as render target */
    void bind();
    /** Bind zero to fbo-state */
    void unbind();

private:

    int p_createTex_();

    QOpenGLContext* p_ctx_;
    QSize p_size_;
    int p_fbo_, p_tex1_, p_tex2_;
};

#endif // FRAMEBUFFEROBJECT_H
