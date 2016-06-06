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

#include "shadertoyapi.h"
#include "shadertoyshader.h"
#include "settings.h"
#include "log.h"

struct ShadertoyApi::Private
{
    Private(ShadertoyApi* p)
        : p         (p)
        , net       (nullptr)
        , loop      (nullptr)
    { }

    void postRequest(const QString& url, std::function<void(QNetworkReply*)>);
    void readShaderList(QNetworkReply* reply);
    void readShader(QNetworkReply* reply);
    bool storeJson(const QString& filename, const QJsonObject& data);
    QImage loadImage(QByteArray);
    QImage loadImage(const QString& src);

    static QString removeFilename(const QString&);

    ShadertoyApi* p;
    QNetworkAccessManager* net;

    QString
        apiUrl, appKey,
        cacheUrlShader, cacheUrlAssets;

    QStringList shaderIds, tempShaderIds;
    ShadertoyShader shader;

    QEventLoop* loop;
};

ShadertoyApi::ShadertoyApi(QObject* parent)
    : QObject       (parent)
    , p_            (new Private(this))
{
    ST_DEBUG_CTOR("ShadertoyApi");
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
const ShadertoyShader& ShadertoyApi::shader() const { return p_->shader; }

void ShadertoyApi::downloadShaderList()
{
    p_->postRequest(p_->apiUrl + "shaders?key=" + p_->appKey,
                    [=](QNetworkReply*r) { p_->readShaderList(r); });
}

void ShadertoyApi::downloadShader(const QString &id)
{
    p_->postRequest(p_->apiUrl + "shaders/" + id + "?key=" + p_->appKey,
                    [=](QNetworkReply*r) { p_->readShader(r); });

}

void ShadertoyApi::Private::postRequest(
        const QString& url, std::function<void(QNetworkReply*)> foo)
{
    ST_DEBUG2("ShadertoyApi::postRequest('" << url << "')");

    if (!net)
    {
        net = new QNetworkAccessManager(p);
    }

    auto r = QNetworkRequest(QUrl(url));

    auto reply = net->get(r);
    connect(reply, &QNetworkReply::finished, [=]()
    {
        foo(reply);
        reply->deleteLater();
    });
}

void ShadertoyApi::Private::readShaderList(QNetworkReply* reply)
{
    ST_DEBUG2("ShadertoyApi:: shader list received");

    // parse json on-the-fly
    auto json = QJsonDocument::fromJson(
                reply->readAll()).object();
    auto array = json.value("Results").toArray();
    shaderIds.clear();
    for (const QJsonValue& v : array)
    {
        auto s = v.toString();
        if (!s.isEmpty())
            shaderIds << s;
    }

    emit p->shaderListReceived();
}

void ShadertoyApi::Private::readShader(QNetworkReply* reply)
{
    ST_DEBUG2("ShadertoyApi:: shader received");

    auto all = reply->readAll();

    auto json = QJsonDocument::fromJson(all).object();
    auto obj = json.value("Shader").toObject();

    shader.setJsonData(obj);

    if (shader.isValid())
    {
        storeJson(shaderIdToFilename(shader.info().id) + ".json",
                  shader.jsonData());
        emit p->shaderReceived();
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

bool ShadertoyApi::loadShader(const QString& id)
{
    ST_DEBUG2("ShadertoyApi::loadShader('" << id << "')");

    QFile file(p_->cacheUrlShader + shaderIdToFilename(id) + ".json");
    if (!file.open(QFile::Text | QFile::ReadOnly))
        return false;

    auto data = file.readAll();
    auto obj = QJsonDocument::fromJson(data).object();
    p_->shader.setJsonData(obj);

    return p_->shader.isValid();
}


void ShadertoyApi::downloadAll()
{
    ST_DEBUG2("ShadertoyApi::downloadAll()");

    connect(this, SIGNAL(shaderListReceived()),
            this, SLOT(p_onList_()), Qt::QueuedConnection);
    connect(this, SIGNAL(shaderReceived()),
            this, SLOT(p_onShader_()), Qt::QueuedConnection);
    downloadShaderList();
}

void ShadertoyApi::p_onList_()
{
    if (!p_->shaderIds.isEmpty())
    {
        p_->tempShaderIds = p_->shaderIds;
        downloadShader(p_->tempShaderIds[0]);
    }
}

void ShadertoyApi::p_onShader_()
{
    ST_DEBUG("ShadertoyApi:: received shader "
             << (p_->shaderIds.size() - p_->tempShaderIds.size())
             << "/" << p_->shaderIds.size()
             << " " << p_->shader.info().name);

    p_->tempShaderIds.pop_front();
    if (!p_->tempShaderIds.isEmpty())
    {
        downloadShader(p_->tempShaderIds[0]);
    }
}

QString ShadertoyApi::Private::removeFilename(const QString& fn)
{
    int idx = fn.lastIndexOf('/');
    if (idx > 0)
        return fn.left(idx);
    return fn;
}

void ShadertoyApi::getTexture(const QString &src)
{
    QImage img = p_->loadImage(src);
    if (!img.isNull())
    {
        ST_INFO("ShadertoyApi:: loaded '" << src << "'");
        emit textureReceived(src, img);
        return;
    }
    const QString url = "https://www.shadertoy.com" + src;

    ST_DEBUG("ShadertoyApi:: downloading texture '" << url << "'");
    p_->postRequest(url, [=](QNetworkReply*r)
    {
        auto data = r->readAll();
        auto img = p_->loadImage(data);
        if (!img.isNull())
        {
            const QString fn = p_->cacheUrlAssets + src,
                          path = p_->removeFilename(fn);
            if (!QDir(".").mkpath(path))
                ST_ERROR("ShadertoyApi:: could not create directory '"
                         << path << "'");
            if (!img.save(fn))
                ST_ERROR("ShadertoyApi:: could not save texture '" << fn << "'")
            else
                ST_INFO("ShadertoyApi:: saved texture '" << fn << "'");
            emit textureReceived(src, img);
        }
    });
}

QImage ShadertoyApi::Private::loadImage(QByteArray d)
{
    QBuffer buf(&d);
    buf.open(QIODevice::ReadOnly);
    QImageReader read(&buf);
    QImage img = read.read();
    if (img.isNull())
        ST_ERROR("load image failed, " << read.errorString());
    return img;
}

QImage ShadertoyApi::Private::loadImage(const QString& src)
{
    QImageReader read(cacheUrlAssets + src);
    QImage img = read.read();
    if (img.isNull())
        ST_ERROR("load image failed '" << read.fileName() << "\n"
                 << read.errorString());
    return img;
}
