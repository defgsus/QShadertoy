/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#ifndef SHADERTOYAPI_H
#define SHADERTOYAPI_H

#include <QObject>

class QNetworkReply;
class ShadertoyShader;

class ShadertoyApi : public QObject
{
    Q_OBJECT
public:
    ShadertoyApi(QObject* parent = nullptr);
    ~ShadertoyApi();

    /** All known IDs, from web or local */
    const QStringList& shaderIds() const;
    /** Is the shader id known? */
    bool hasShader(const QString& id) const;
    /** Receive shader for ID, or invalid shader if unknown */
    ShadertoyShader getShader(const QString& id) const;

    /** Converts shader id to case-insensitive filename */
    static QString shaderIdToFilename(const QString& id);

signals:

    /** All internal changes are reflected here */
    void shaderListChanged();

    /** The shaderId() list is updated */
    void shaderListReceived();
    /** A shader has been downloaded and saved */
    void shaderReceived(const QString& id);
    /** A texture has been downloaded or loaded from cache */
    void textureReceived(const QString& src, const QImage&);

    void downloadProgress(double percent);
    void mergeFinished();

public slots:

    // ----- web api -------

    /** Update the shaderIds() list */
    void downloadShaderList();
    /** Download a specific shader by ID */
    void downloadShader(const QString& id);

    /** Tries to download every shader that is not in shaderIds()
        or is in shaderIds() and not valid. */
    void mergeWithWeb();

    /** Download a texture or load from cache */
    void getTexture(const QString& src);

    void stopRequests();

    // ---- offline api ----

    /** Load the shaderId() list from cache */
    bool loadShaderList();
    /** Tries to load all shaders in the list from cache */
    void loadAllShaders();
    /** Loads a specific shader by ID from cache */
    bool loadShader(const QString& id);


private slots:

    void p_onReply_(QNetworkReply*);

private:
    struct Private;
    Private* p_;
};

#endif // SHADERTOYAPI_H
