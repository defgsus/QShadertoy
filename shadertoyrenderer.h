/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef SHADERTOYRENDERER_H
#define SHADERTOYRENDERER_H

#include <QObject>

class QOpenGLContext;
class QSurface;
class ShadertoyShader;

class ShadertoyRenderer : public QObject
{
    Q_OBJECT
public:
    explicit ShadertoyRenderer(QObject *parent = 0);
    explicit ShadertoyRenderer(QOpenGLContext* ctx, QObject *parent = 0);
    explicit ShadertoyRenderer(QOpenGLContext* ctx, QSurface*, QObject *parent = 0);
    ~ShadertoyRenderer();

    /** Shader is compiled? */
    bool isReady() const;

    /** The description of the last error occured
        during compilation or rendering */
    const QString& errorString() const;

signals:

    /** Emitted when a texture is loaded */
    void rerender();

public slots:

    /** Sets the shader to render.
        The next call to render() will compile the shader */
    void setShader(const ShadertoyShader& );

    /** Sets/changes the resolution for the shader and all
        it's buffers. If the resolution changes, the next call
        to render() will recompile everything. */
    void setResolution(const QSize& );

    void setMouse(const QPoint& pos, bool leftKey, bool rightKey);
    void setGlobalTime(float ti);
    void setFrameNumber(int);
    void setTimeDelta(float);
    void setDate(const QDateTime&);

    /** Renders the shader to the given gl context.
        This might compile the shader if needed.
        On any error, false is returned and errorString() contains
        the description of the error. */
    bool render(const QRect& viewPort);

private slots:

    void p_onTexture_(const QString& src, const QImage& img);

private:
    struct Private;
    Private* p_;
};

#endif // SHADERTOYRENDERER_H
