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
    enum Projection
    {
        P_RECT,
        P_FISHEYE,
        P_CROSS_EYE
    };

    explicit ShadertoyRenderer(QObject *parent = 0);
    explicit ShadertoyRenderer(QOpenGLContext* ctx, QObject *parent = 0);
    explicit ShadertoyRenderer(QOpenGLContext* ctx, QSurface*, QObject *parent = 0);
    ~ShadertoyRenderer();

    /** Shader is compiled? */
    bool isReady() const;

    /** The description of the last error occured
        during compilation or rendering */
    const QString& errorString() const;

    /** Current projection mode for VR hook. */
    Projection projectionMode() const;

    /** Messured frames per second that are archived */
    double messuredFps() const;

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
    /** Sets the projection mode using the VR hook */
    void setProjectionMode(Projection p);

    /** Sets mouse state */
    void setMouse(const QPoint& pos, bool leftKey, bool rightKey);
    /** Key-down or up event for a certain key */
    void setKeyboard(Qt::Key key, bool pressed);
    void setGlobalTime(float ti);
    void setFrameNumber(int);
    void setDate(const QDateTime&);
    void setEyeDistance(float);
    void setEyeRotation(float);

    /** Renders the shader to the current gl context.
        This will compile the shader and create all resources if needed.
        On any error, false is returned and errorString() contains
        the description of the error.
        If @p continuous is true, the iTimeDelta variable is updated
        with the true time delta between this and last call. */
    bool render(const QRect& viewPort, bool continuous);

private slots:

    void p_onTexture_(const QString& src, const QImage& img);

private:
    struct Private;
    Private* p_;
};

#endif // SHADERTOYRENDERER_H
