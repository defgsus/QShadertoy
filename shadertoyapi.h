/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#ifndef SHADERTOYAPI_H
#define SHADERTOYAPI_H

#include <QObject>

class ShadertoyShader;

class ShadertoyApi : public QObject
{
    Q_OBJECT
public:
    ShadertoyApi(QObject* parent = nullptr);
    ~ShadertoyApi();


    const QStringList& shaderIds() const;
    const ShadertoyShader& shader() const;

    /** Converts shader id to case-insensitive filename */
    static QString shaderIdToFilename(const QString& id);
signals:

    void shaderListReceived();
    void shaderReceived();

    void textureReceived(const QString& src, const QImage&);

public slots:

    // ----- web api -------

    void downloadShaderList();
    void downloadShader(const QString& id);

    /** XXX Hacky right now */
    void downloadAll();

    void getTexture(const QString& src);

    // ---- offline api ----

    bool loadShaderList();
    bool loadShader(const QString& id);

private slots:

    void p_onList_();
    void p_onShader_();

private:
    struct Private;
    Private* p_;
};

#endif // SHADERTOYAPI_H
