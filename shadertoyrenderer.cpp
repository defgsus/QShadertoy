/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include <iostream>

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurface>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QVector4D>
#include <QMatrix4x4>
#include <QImage>
#include <QSet>

#include "shadertoyrenderer.h"
#include "shadertoyshader.h"
#include "shadertoyapi.h"
#include "log.h"

#define STRENDER_ERROR(qstring__) \
    errorStr = qstring__; \
    ST_ERROR("ShadertoyRenderer: " << errorStr);


struct ShadertoyRenderer::Private
{
    Private(ShadertoyRenderer* p)
        : p             (p)
        , resolution    (256, 256)
        , projectionMode(P_RECT)
        , api           (new ShadertoyApi(p))
        , context       (nullptr)
        , surface       (nullptr)
        , bufVert       (nullptr)
        , bufIdx        (nullptr)
        , mouseData     (0.f, 0.f, 0.f, 0.f)
        , dateData      (0.f, 0.f, 0.f, 0.f)
        , globalTime    (0.f)
        , timeDelta     (0.f)
        , frameNumber   (0)
        , needsRecompile(true)
    {
        connect(api, SIGNAL(textureReceived(QString,QImage)),
                p, SLOT(p_onTexture_(QString,QImage)),
                Qt::QueuedConnection);
    }

    struct RenderPass;

    bool createGl();
    void destroyGl();
    QOpenGLBuffer* createBuffer(
            QOpenGLBuffer::Type, const void* data, int count);
    bool render(const QRect& viewPort);
    bool drawQuad(RenderPass& pass);

    struct RenderPass
    {
        QOpenGLShaderProgram* shader;
        QOpenGLFramebufferObject* fbo;
        QOpenGLTexture* tex[4];
        QString src[4];
        bool vFlip[4];
        QOpenGLTexture::WrapMode wrapMode[4];
        QOpenGLTexture::Filter filterType[4];
        QImage img[4];

        int mvp_matrix,
            a_position,
            iResolution,
            iGlobalTime,
            iTimeDelta,
            iFrame,
            iChannelTime,
            iChannelResolution,
            iMouse,
            iDate,
            iSampleRate,
            iChannel[4];
    };

    const static GLfloat quadVertices[];
    const static GLushort quadIndices[];

    ShadertoyRenderer* p;

    QSize resolution;
    Projection projectionMode;
    QString errorStr;

    ShadertoyApi* api;

    QOpenGLContext* context;
    QSurface* surface;

    QOpenGLBuffer* bufVert, *bufIdx;
    QMatrix4x4 projection;
    QVector4D mouseData, dateData;
    float globalTime, timeDelta;
    int frameNumber;

    ShadertoyShader shadertoy;
    std::vector<RenderPass> passes;


    bool needsRecompile;
};

const GLfloat ShadertoyRenderer::Private::quadVertices[] =
{ -1.f, -1.f,
   1.f, -1.f,
   1.f,  1.f,
  -1.f,  1.f };
const GLushort ShadertoyRenderer::Private::quadIndices[] =
{ 0, 1, 2,
  0, 2, 3 };


ShadertoyRenderer::ShadertoyRenderer(
        QOpenGLContext* ctx, QSurface* surf, QObject* parent)
    : QObject       (parent)
    , p_            (new Private(this))
{
    ST_DEBUG_CTOR("ShadertoyRenderer");
    p_->context = ctx;
    p_->surface = surf;
}

ShadertoyRenderer::ShadertoyRenderer(QOpenGLContext* ctx, QObject* parent)
    : ShadertoyRenderer(ctx, nullptr, parent)
{ }

ShadertoyRenderer::ShadertoyRenderer(QObject* parent)
    : ShadertoyRenderer(nullptr, nullptr, parent)
{ }

ShadertoyRenderer::~ShadertoyRenderer()
{
    ST_DEBUG_CTOR("~ShadertoyRenderer");
    p_->destroyGl();
    delete p_;
}

bool ShadertoyRenderer::isReady() const
{
    return !p_->needsRecompile && p_->shadertoy.isValid() && !p_->passes.empty();
}

const QString& ShadertoyRenderer::errorString() const { return p_->errorStr; }

void ShadertoyRenderer::setShader(const ShadertoyShader& s)
{
    ST_DEBUG2("ShadertoyRenderer::setShader()");

    p_->shadertoy = s;
    p_->needsRecompile = true;

}

void ShadertoyRenderer::setResolution(const QSize& s)
{
    if (p_->resolution == s)
        return;
    p_->resolution = s;
    p_->needsRecompile = true;
}

void ShadertoyRenderer::setMouse(const QPoint &pos, bool leftKey, bool rightKey)
{
    p_->mouseData = QVector4D(pos.x(), pos.y(),
                              leftKey ? 1.f : 0.f, rightKey ? 1.f : 0.f);
}
void ShadertoyRenderer::setDate(const QDateTime& dt)
{
    auto d = dt.date();
    p_->dateData = QVector4D(d.year(), d.month(), d.day(), dt.time().second());
}

void ShadertoyRenderer::setGlobalTime(float ti) { p_->globalTime = ti; }
void ShadertoyRenderer::setTimeDelta(float ti) { p_->timeDelta = ti; }
void ShadertoyRenderer::setFrameNumber(int f) { p_->frameNumber = f; }
void ShadertoyRenderer::setProjection(Projection p)
{
    if (p_->projectionMode == p)
        return;
    p_->projectionMode = p;
    p_->needsRecompile = true;
}


QOpenGLBuffer* ShadertoyRenderer::Private::createBuffer(
        QOpenGLBuffer::Type type, const void *data, int count)
{
    auto buf = new QOpenGLBuffer(type);
    buf->setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (!buf->create())
    {
        STRENDER_ERROR("Could not create buffer object");
        delete buf;
        return nullptr;
    }
    if (!buf->bind())
    {
        STRENDER_ERROR("Could not bind buffer object");
        delete buf;
        return nullptr;
    }
    buf->allocate(data, count);
    return buf;
}

bool ShadertoyRenderer::Private::createGl()
{
    ST_DEBUG2("ShadertoyRenderer::createGl()");

    const QString
              vertSrc =
"#ifdef GL_ES\n"
"precision highp int;\n"
"precision highp float;\n"
"#endif\n"
"\n"
"uniform mat4 mvp_matrix;\n"
"attribute vec4 a_position;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = mvp_matrix * a_position;\n"
"}\n"
            , fragSrc1 =
"#ifdef GL_ES\n"
"precision highp int;\n"
"precision highp float;\n"
"#endif\n"
"uniform vec3  iResolution;              // resolution of output texture in pixels\n"
"uniform float iGlobalTime;              // scene time in seconds\n"
"uniform float iTimeDelta;               // time between this and last frame\n"
"uniform int   iFrame;                   // current frame counter\n"
"uniform float iChannelTime[4];          // playback of channel in seconds\n"
"uniform vec3  iChannelResolution[4];    // resolution per channel in pixels\n"
"uniform vec4  iMouse;                   // xy = pos in pixels, zw = buttons\n"
"uniform vec4  iDate;                    // year, month, day, time in seconds\n"
"uniform float iSampleRate;              // sound sampling rate in Hertz\n"
                , fragSrc2 =
"void main()\n"
"{\n"
"    mainImage(gl_FragColor, gl_FragCoord.xy);\n"
//"    gl_FragColor = vec4(gl_FragCoord.xy / iResolution.xy, 0,1);\n"
"}\n"
            , fragSrcVr =
"void main()\n"
"{\n"
"    vec2 uv = (gl_FragCoord.xy - .5*iResolution.xy) / iResolution.y * 2.;\n"
"    vec3 ro = vec3(0.);\n"
"    vec3 rd = normalize(vec3(uv, -2. + length(uv)));\n"
"    mainVR(gl_FragColor, gl_FragCoord.xy, ro, rd);\n"
"}\n"
    ;

    errorStr.clear();

    if (!context)
    {
        STRENDER_ERROR(tr("No context set"));
        return false;
    }

    if (surface)
    {
        if (!context->makeCurrent(surface))
        {
            STRENDER_ERROR(tr("Can not make context current"));
            return false;
        }
    }

    projection.setToIdentity();
    projection.ortho(QRectF(-1,-1,2,2));

    // --- create shader passes ---

    for (size_t i=0; i<shadertoy.numRenderPasses(); ++i)
    {
        const ShadertoyRenderPass& pass = shadertoy.renderPass(i);

        // XXX todo
        if (pass.type() == ShadertoyRenderPass::T_SOUND)
            continue;

        // -- create per-pass texture input uniform code --

        QString fragSrc1b;
        for (size_t j=0; j<4; ++j)
        {
            fragSrc1b += "uniform sampler";
            if (j < pass.numInputs()
                && pass.input(j).type == ShadertoyInput::T_CUBEMAP)
                fragSrc1b += "Cube";
            else
                fragSrc1b += "2D";
            fragSrc1b += QString(" iChannel%1;\n").arg(j);
        }

        // -- create objects and install at once --
        // so destroyGl() can dealloc it all on errors below
        RenderPass rpNew;
        rpNew.shader = new QOpenGLShaderProgram();
        rpNew.fbo = nullptr;
        if (pass.type() != ShadertoyRenderPass::T_IMAGE)
            rpNew.fbo = new QOpenGLFramebufferObject(320, 200, GL_TEXTURE_2D);

        passes.push_back(rpNew);
        RenderPass& rp = passes.back();

        // --- query textures ---

        QSet<QString> queried;
        for (size_t j=0; j<4; ++j)
        {
            rp.tex[j] = nullptr;
            rp.src[j].clear();
            rp.vFlip[j] = false;
            rp.wrapMode[j] = QOpenGLTexture::ClampToEdge;
            rp.filterType[j] = QOpenGLTexture::Linear;

            if (j < pass.numInputs())
            {
                auto inp = pass.input(j);
                //ST_INFO(inp.toString());

                if (inp.type == ShadertoyInput::T_TEXTURE
                || inp.type == ShadertoyInput::T_CUBEMAP
                || inp.type == ShadertoyInput::T_BUFFER)
                {
                    rp.src[j] = inp.src;
                    rp.vFlip[j] = inp.vFlip;
                    if (inp.filterType == ShadertoyInput::F_NEAREST)
                        rp.filterType[j] = QOpenGLTexture::Nearest;
                    else if (inp.filterType == ShadertoyInput::F_MIPMAP)
                        rp.filterType[j] = QOpenGLTexture::LinearMipMapLinear;
                    if (inp.wrapMode == ShadertoyInput::W_REPEAT)
                        rp.wrapMode[j] = QOpenGLTexture::Repeat;
                    if (!queried.contains(inp.src))
                    {
                        api->getTexture(inp.src);
                        queried.insert(inp.src);
                    }
                }
            }
        }

        // -- compile shader --

        auto vert = new QOpenGLShader(QOpenGLShader::Vertex, rp.shader);
        if (!vert->compileSourceCode(vertSrc))
        {
            STRENDER_ERROR(tr("vertex compile failed (pass: %1):\n%2")
                           .arg(pass.name())
                           .arg(vert->log())
                           );
            destroyGl();
            return false;
        }

        auto frag = new QOpenGLShader(QOpenGLShader::Fragment, rp.shader);
        QString src =
            fragSrc1 + fragSrc1b + pass.fragmentSource();
        if (projectionMode == P_RECT)
            src += fragSrc2;
        else
            src += fragSrcVr;
        if (!frag->compileSourceCode(src))
        {
            STRENDER_ERROR(tr("compile failed (pass: %1):\n%2")
                           .arg(pass.name())
                           .arg(frag->log())
                           );
            destroyGl();
            return false;
        }

        // -- link shader --

        if (   !rp.shader->addShader(vert)
            || !rp.shader->addShader(frag)
            || !rp.shader->link())
        {
            STRENDER_ERROR(tr("link failed:\n") + rp.shader->log());
            destroyGl();
            return false;
        }

        // -- get attributes and uniforms --

        rp.a_position = rp.shader->attributeLocation("a_position");
        rp.mvp_matrix = rp.shader->uniformLocation("mvp_matrix");
        rp.iResolution = rp.shader->uniformLocation("iResolution");
        rp.iGlobalTime = rp.shader->uniformLocation("iGlobalTime");
        rp.iTimeDelta = rp.shader->uniformLocation("iTimeDelta");
        rp.iFrame = rp.shader->uniformLocation("iFrame");
        rp.iChannelTime = rp.shader->uniformLocation("iChannelTime[0]");
        rp.iChannelResolution =
                rp.shader->uniformLocation("iChannelResolution[0]");
        rp.iMouse = rp.shader->uniformLocation("iMouse");
        rp.iDate = rp.shader->uniformLocation("iDate");
        rp.iSampleRate = rp.shader->uniformLocation("iSampleRate");
        for (int j=0; j<4; ++j)
        {
            rp.iChannel[j] = rp.shader->uniformLocation(
                                    QString("iChannel%1").arg(j));
            rp.shader->setUniformValue(rp.iChannel[j], j);
        }

    }

    // -------- create screen quad geometry ----------

    bufVert = createBuffer(QOpenGLBuffer::VertexBuffer, quadVertices,
                           4 * 2 * sizeof(GLfloat));
    if (!bufVert) { destroyGl(); return false; }
    bufIdx = createBuffer(QOpenGLBuffer::IndexBuffer, quadIndices,
                           6 * sizeof(GLushort));
    if (!bufIdx) { destroyGl(); return false; }


    // -- done --

    needsRecompile = false;
    return true;
}


void ShadertoyRenderer::Private::destroyGl()
{
    ST_DEBUG2("ShadertoyRenderer::destroyGl()");

    if (!context)
        return;
    if (surface)
    {
        context->makeCurrent(surface);
    }

    if (bufIdx && bufIdx->isCreated())
        bufIdx->release();
    delete bufIdx;
    bufIdx = nullptr;

    if (bufVert && bufVert->isCreated())
        bufVert->release();
    delete bufVert;
    bufVert = nullptr;

    for (RenderPass& rp : passes)
    {
        for (int i=0; i<4; ++i)
        {
            if (rp.tex[i] && rp.tex[i]->isCreated())
                rp.tex[i]->destroy();
            delete rp.tex[i];
        }
        if (rp.shader && rp.shader->isLinked())
            rp.shader->release();
        delete rp.shader;
        if (rp.fbo && rp.fbo->isValid())
            rp.fbo->release();
        delete rp.fbo;
    }
    passes.clear();
}

void ShadertoyRenderer::p_onTexture_(const QString &src, const QImage &img)
{
    for (Private::RenderPass& pass : p_->passes)
    {
        for (int i=0; i<4; ++i)
            if (pass.src[i] == src)
                pass.img[i] = img;
    }
    emit rerender();
}


bool ShadertoyRenderer::render(const QRect& v) { return p_->render(v); }

bool ShadertoyRenderer::Private::render(const QRect& viewPort)
{
    if (!p->isReady() || needsRecompile)
    {
        destroyGl();
        createGl();
    }
    if (!p->isReady())
        return false;

    auto gl = context->functions();
    if (!gl)
    {
        STRENDER_ERROR("No OpenGL functions object in context");
        return false;
    }

    gl->glViewport(viewPort.x(), viewPort.y(), viewPort.width(), viewPort.height());

    return drawQuad(passes[0]);
}

bool ShadertoyRenderer::Private::drawQuad(RenderPass& pass)
{
    auto gl = context->functions();

    if (!pass.shader->bind())
    {
        STRENDER_ERROR("Could not bind shader");
        return false;
    }

    // --- bind textures ---

    for (int i=0; i<4; ++i)
    {
        if (pass.tex[i] == nullptr
            && !pass.img[i].isNull())
        {
            pass.tex[i] = new QOpenGLTexture(
                        pass.vFlip[i] ? pass.img[i].mirrored(false, true)
                                      : pass.img[i],
                        pass.filterType[i] == QOpenGLTexture::LinearMipMapLinear
                        ? QOpenGLTexture::GenerateMipMaps
                        : QOpenGLTexture::DontGenerateMipMaps);
            pass.tex[i]->setMinMagFilters(
                QOpenGLTexture::Linear, pass.filterType[i]);
            pass.tex[i]->setWrapMode(pass.wrapMode[i]);
        }

        if (!pass.tex[i])
            continue;

        gl->glActiveTexture(GL_TEXTURE0 + i);
        pass.tex[i]->bind();
    }


    // --- bind geometry ---

    if (!bufVert->bind())
    {
        STRENDER_ERROR("Could not bind vertex buffer");
        return false;
    }
    if (!bufIdx->bind())
    {
        STRENDER_ERROR("Could not bind index buffer");
        return false;
    }

    // --- update attributes and uniforms ---

    pass.shader->setUniformValue(pass.mvp_matrix, projection);
    pass.shader->setUniformValue(pass.iResolution,
                                 float(resolution.width()),
                                 float(resolution.height()),
                                 float(resolution.width()) / resolution.height());
    pass.shader->setUniformValue(pass.iMouse, mouseData);
    pass.shader->setUniformValue(pass.iGlobalTime, globalTime);
    pass.shader->setUniformValue(pass.iFrame, frameNumber);
    pass.shader->setUniformValue(pass.iTimeDelta, timeDelta);
    pass.shader->setUniformValue(pass.iDate, dateData);

    // -- bind vertex array --

    pass.shader->enableAttributeArray(pass.a_position);
    pass.shader->setAttributeBuffer(pass.a_position, GL_FLOAT, 0, 2, 0);

    // --- render ---

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glGetError();
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    auto e = gl->glGetError();
    if (e != 0)
    {
        STRENDER_ERROR("drawElements failed");
        return false;
    }

    return true;
}
