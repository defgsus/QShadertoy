/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/


#include <functional>

#include <QDebug>

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QByteArray>
#include <QRegExp>
#include <QThread>

#include "shadertoyapi.h"
#include "shadertoyshader.h"
#include "settings.h"
#include "log.h"

struct ShadertoyApi::Private
{
    Private(ShadertoyApi* p)
        : p             (p)
        , net           (nullptr)
        , numDownloads  (0)
        , doWebMerge    (false)
    { }

    void postRequest(const QString& url, const QVariant& userData);
    void readShaderList(QNetworkReply* reply);
    void readShader(QNetworkReply* reply);
    void readAsset(QNetworkReply* reply);

    bool storeJson(const QString& filename, const QJsonObject& data);
    QImage loadImage(QByteArray);
    QImage loadImage(const QString& src);
    bool loadShader(const QString& id);

    static QString removeFilename(const QString&);

    ShadertoyApi* p;
    QNetworkAccessManager* net;

    QString
        apiUrl, appKey,
        cacheUrlShader, cacheUrlAssets;

    QStringList shaderIds;
    QSet<QString> downloadShaderIds;
    int numDownloads;
    QMap<QString, ShadertoyShader> shaderMap;

    bool doWebMerge;
};

namespace {

}

ShadertoyApi::ShadertoyApi(QObject* parent)
    : QObject       (parent)
    , p_            (new Private(this))
{
    ST_DEBUG_CTOR("ShadertoyApi()");
    p_->apiUrl = "https://www.shadertoy.com/api/v1/";
    p_->appKey = "rtHtwr";
    //p_->appKey = Settings::instance().value(
    //                Settings::keyAppkey, "rtHtwr").toString();
    p_->cacheUrlShader = "./shader/";
    p_->cacheUrlAssets = "./assets"; ///< no trailing / !
}

ShadertoyApi::~ShadertoyApi()
{
    ST_DEBUG_CTOR("~ShadertoyApi");
    stopRequests();
    delete p_;
}

QString ShadertoyApi::shaderIdToFilename(const QString& id)
{
    QString s;
    for (auto c : id)
    {
        s += c;
        if (c.isUpper())
            s += "_";
    }
    return s;
}

const QStringList& ShadertoyApi::shaderIds() const { return p_->shaderIds; }
bool ShadertoyApi::hasShader(const QString& id) const
{
    return p_->shaderMap.contains(id);
}
ShadertoyShader ShadertoyApi::getShader(const QString& id) const
{
    auto i = p_->shaderMap.find(id);
    return i != p_->shaderMap.end() ? i.value() : ShadertoyShader(id);
}


void ShadertoyApi::downloadShaderList()
{
    p_->postRequest(p_->apiUrl + "shaders?key=" + p_->appKey, "shaderlist");
}

void ShadertoyApi::downloadShader(const QString &id)
{
    p_->postRequest(p_->apiUrl + "shaders/" + id + "?key=" + p_->appKey,
                    "shader");
}

void ShadertoyApi::stopRequests()
{
    p_->doWebMerge = false;
    p_->downloadShaderIds.clear();
}

void ShadertoyApi::Private::postRequest(
        const QString& url, const QVariant& userData)
{
    ST_DEBUG2("ShadertoyApi::postRequest('" << url << "')");

    if (!net)
    {
        net = new QNetworkAccessManager(p);
        connect(net, SIGNAL(finished(QNetworkReply*)),
                p, SLOT(p_onReply_(QNetworkReply*)), Qt::QueuedConnection);
    }

    auto r = QNetworkRequest(QUrl(url));

    auto reply = net->get(r);
    reply->setProperty("user", userData);
}

void ShadertoyApi::p_onReply_(QNetworkReply* reply)
{
    const auto user = reply->property("user").toString();
    if (user == "shader")
        p_->readShader(reply);
    else if (user == "shaderlist")
        p_->readShaderList(reply);
    else if (user.startsWith("asset"))
        p_->readAsset(reply);

    reply->deleteLater();
}



void ShadertoyApi::Private::readShaderList(QNetworkReply* reply)
{
    ST_DEBUG2("ShadertoyApi:: shader list received");

    QStringList ids;

    // parse json on-the-fly
    auto json = QJsonDocument::fromJson(
                reply->readAll()).object();
    auto array = json.value("Results").toArray();
    ids.clear();
    for (const QJsonValue& v : array)
    {
        auto s = v.toString();
        if (!s.isEmpty())
            ids << s;
    }

    if (!doWebMerge)
    {
        shaderIds = ids;
        emit p->shaderListReceived();
        emit p->shaderListChanged();
        return;
    }

    // merge id list
    auto mergeIds = shaderIds.toSet() += ids.toSet();
    shaderIds = mergeIds.toList();

    // find invalid shader
    downloadShaderIds.clear();
    for (auto& id : shaderIds)
    {
        auto s = p->getShader(id);
        if (!s.isValid())
            downloadShaderIds << id;
    }

    // trigger download
    if (!downloadShaderIds.isEmpty())
    {
        numDownloads = downloadShaderIds.size();
        p->downloadShader(*downloadShaderIds.begin());
    }
}


void ShadertoyApi::Private::readShader(QNetworkReply* reply)
{
    ST_DEBUG2("ShadertoyApi:: shader received");

    auto all = reply->readAll();

    auto json = QJsonDocument::fromJson(all).object();
    auto obj = json.value("Shader").toObject();

    ShadertoyShader shader;
    shader.setJsonData(obj);

    if (shader.isValid())
    {
        storeJson(shaderIdToFilename(shader.info().id) + ".json",
                  shader.jsonData());

        shaderMap[shader.info().id] = shader;

        emit p->shaderReceived(shader.info().id);
        emit p->shaderListChanged();
    }

    if (numDownloads)
    {
        emit p->downloadProgress(100 -
                std::min(100, downloadShaderIds.size() * 100 / numDownloads));

        // grab the next shader from requests
        downloadShaderIds.remove(shader.info().id);
        if (!downloadShaderIds.isEmpty())
        {
            p->downloadShader(*downloadShaderIds.begin());
        }
        else
        {
            numDownloads = 0;
            emit p->mergeFinished();
        }
    }
}

void ShadertoyApi::Private::readAsset(QNetworkReply* reply)
{
    auto data = reply->readAll();
    if (data.isEmpty())
    {
        ST_ERROR("No data received");
        return;
    }

    const QString
            src = reply->property("user").toString().mid(5),
            fn = cacheUrlAssets + src,
            path = removeFilename(fn);
    if (!QDir(".").mkpath(path))
    {
        ST_ERROR("ShadertoyApi:: could not create directory '"
                 << path << "'");
        return;
    }

    // save locally
    QFile file(fn);
    if (!file.open(QFile::WriteOnly))
    {
        ST_ERROR("ShadertoyApi:: could not save asset '" << fn << "'");
        return;
    }
    file.write(data);
    ST_DEBUG2("ShadertoyApi:: saved asset '" << fn << "'");

    // it's an image?
    auto img = loadImage(data);
    if (!img.isNull())
    {
        ST_INFO("ShadertoyApi:: received texture '" << fn << "'");
        emit p->textureReceived(src, img);
    }
    else
    {
        ST_INFO("ShadertoyApi:: received asset '" << fn << "'");
        emit p->assetReceived(src);
    }
}



bool ShadertoyApi::Private::storeJson(
        const QString &filename, const QJsonObject &data)
{
    ST_DEBUG2("ShadertoyApi::storeJson('" << filename << "', json)");

    if (!QDir(".").mkpath(cacheUrlShader))
        return false;

    QFile file(cacheUrlShader + filename);
    if (!file.open(QFile::Text | QFile::WriteOnly))
        return false;

    file.write(QJsonDocument(data).toJson());

    return true;
}


void ShadertoyApi::getAsset(const QString &src)
{
    ST_DEBUG2("ShadertoyApi::getAsset(" << src << ")");

    if (QFileInfo(p_->cacheUrlAssets + src).exists())
    {
        QImage img = p_->loadImage(src);
        if (!img.isNull())
        {
            ST_INFO("ShadertoyApi:: loaded '" << src << "'");
            emit textureReceived(src, img);
            return;
        }
        ST_WARN("Non-images not implemented yet '" << src << "'");
        return;
    }

    const QString url = "https://www.shadertoy.com" + src;

    ST_DEBUG("ShadertoyApi:: downloading asset '" << url << "'");
    p_->postRequest(url, "asset" + src);
}






bool ShadertoyApi::loadShaderList()
{
    ST_DEBUG2("ShadertoyApi::loadShaderList()");

    QDir dir(p_->cacheUrlShader);

    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot
                  | QDir::Readable  | QDir::Hidden);
    dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

    dir.setNameFilters(QStringList() << "*.json");

    QStringList files = dir.entryList();

    p_->shaderIds.clear();
    for (const QString& fn : files)
    {
        p_->shaderIds << fn.left(fn.size() - 5).remove(QRegExp("_"));
    }

    return !p_->shaderIds.isEmpty();
}

void ShadertoyApi::loadAllShaders()
{
    ST_DEBUG2("ShadertoyApi::loadAllShaders()");

    for (auto& id : p_->shaderIds)
        p_->loadShader(id);

    emit shaderListChanged();
}

bool ShadertoyApi::loadShader(const QString& id)
{
    ST_DEBUG2("ShadertoyApi::loadShader('" << id << "')");

    if (!p_->loadShader(id))
        return false;

    emit shaderListChanged();
    return true;
}

bool ShadertoyApi::Private::loadShader(const QString& id)
{
    ST_DEBUG2("ShadertoyApi::Private::loadShader('" << id << "')");

    QString fn = cacheUrlShader + shaderIdToFilename(id) + ".json";

    if (!QFileInfo(id).exists(fn))
    {
        ST_DEBUG2("Shader does not exist '" << fn << "'");
        return false;
    }

    QFile file(fn);
    if (!file.open(QFile::Text | QFile::ReadOnly))
    {
        ST_ERROR("Could not open shader file '" << file.fileName() << "', "
                 << file.errorString());
        return false;
    }

    auto data = file.readAll();
    auto obj = QJsonDocument::fromJson(data).object();

    ShadertoyShader shader;
    if (!shader.setJsonData(obj) || !shader.isValid())
    {
        ST_ERROR("Error parsing json file '" << file.fileName() << "'");
        return false;
    }

    ST_DEBUG2("ShadertoyApi: insert shader '" << shader.info().id << "'");

    shaderMap[shader.info().id] = shader;
    return shader.isValid();
}



void ShadertoyApi::mergeWithWeb()
{
    ST_DEBUG2("ShadertoyApi::mergeWithWeb()");

    p_->doWebMerge = true;
    downloadShaderList();
}




QString ShadertoyApi::Private::removeFilename(const QString& fn)
{
    int idx = fn.lastIndexOf('/');
    if (idx > 0)
        return fn.left(idx);
    return fn;
}



QImage ShadertoyApi::Private::loadImage(QByteArray d)
{
    QBuffer buf(&d);
    buf.open(QIODevice::ReadOnly);
    QImageReader read(&buf);
    QImage img = read.read();
    if (img.isNull())
        ST_ERROR("load image failed: " << read.errorString());
    return img;
}

QImage ShadertoyApi::Private::loadImage(const QString& src)
{
    QImageReader read(cacheUrlAssets + src);
    QImage img = read.read();
    if (img.isNull())
        ST_ERROR("load image failed for '" << read.fileName() << "': "
                 << read.errorString());
    return img;
}
