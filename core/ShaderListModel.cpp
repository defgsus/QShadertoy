/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include <QList>
#include <QColor>
#include <QPixmap>
#include <QImage>

#include "ShaderListModel.h"
#include "ShadertoyApi.h"
#include "ShadertoyShader.h"
#include "log.h"

struct ShaderListModel::Private
{
    Private(ShaderListModel* p)
        : p     (p)
        , api   (new ShadertoyApi(p))
        , doThumbnails  (false)
    {
        api->loadShaderList();
        connect(api, &ShadertoyApi::shaderListChanged, [=]()
        {
            copyListFromApi();
        });
    }

    void initHeaders();
    void copyListFromApi();

    struct Column
    {
        Column() { }
        Column(const QString& n, ColumnId id)
            : name(n), id(id) { }
        QString name;
        ColumnId id;
    };

    ShaderListModel* p;
    ShadertoyApi* api;
    QList<ShadertoyShader> shaders;
    QVector<Column> columns;
    QMap<QString, QPixmap> pixMap;
    bool doThumbnails;
};

ShaderListModel::ShaderListModel(QObject *parent)
    : QAbstractTableModel   (parent)
    , p_        (new Private(this))
{
    p_->initHeaders();
}

void ShaderListModel::Private::initHeaders()
{
    columns.clear();

    columns << Column(tr("id"), C_ID);
    if (doThumbnails)
        columns << Column(tr("pic"), C_IMAGE);
    columns << Column(tr("name"), C_NAME)
            << Column(tr("user"), C_USER)
            << Column(tr("date"), C_DATE)
            << Column(tr("views"), C_VIEWS)
            << Column(tr("likes"), C_LIKES)
            << Column(tr("passes"), C_PASSES)
            << Column(tr("chars"), C_NUM_CHARS)
            << Column(tr("has sound"), C_HAS_SOUND)
            << Column(tr("use texture"), C_USE_TEX)
            << Column(tr("use keyboard"), C_USE_KEYBOARD)
            << Column(tr("use mouse"), C_USE_MOUSE)
            << Column(tr("use music"), C_USE_MUSIC)
            << Column(tr("flags"), C_FLAGS)
               ;
}

ShaderListModel::~ShaderListModel()
{
    delete p_;
}

ShadertoyApi* ShaderListModel::api() { return p_->api; }

const QStringList& ShaderListModel::shaderIds() const
    { return p_->api->shaderIds(); }

void ShaderListModel::Private::copyListFromApi()
{
    ST_DEBUG2("ShaderListModel::copyListFromApi()");

    p->beginResetModel();

    shaders.clear();
    for (auto id : api->shaderIds())
        shaders << api->getShader(id);

    p->endResetModel();
}

void ShaderListModel::setEnableThumbnails(bool e)
{
    beginResetModel();
    p_->doThumbnails = e;
    p_->initHeaders();
    endResetModel();
}

ShadertoyShader ShaderListModel::getShader(const QModelIndex& idx) const
{
    if (idx.row() < 0 || idx.row() >= p_->shaders.size())
        return ShadertoyShader();
    return p_->shaders[idx.row()];
}

const ShadertoyShader& ShaderListModel::getShader(int row) const
{
    return p_->shaders[row];
}

int ShaderListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : p_->columns.size();
}

int ShaderListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : p_->shaders.size();
}

QVariant ShaderListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= p_->shaders.size())
        return QVariant();

    if (index.column() < 0 || index.column() > p_->columns.size())
        return QVariant();
    const Private::Column& column = p_->columns[index.column()];

    const ShadertoyShader shader = p_->shaders[index.row()];

    if (!shader.isValid())
    {
        if (role == Qt::BackgroundColorRole)
            return QColor(255,128,128);
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            if (column.id == C_ID)
                return p_->shaders[index.row()].info().id;
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (column.id)
        {
            case C_ID: return shader.info().id;
            case C_NAME: return shader.info().name;
            case C_USER: return shader.info().username;
            case C_DATE: return shader.info().date;
            case C_VIEWS: return shader.info().views;
            case C_LIKES: return shader.info().likes;
            case C_PASSES: return (int)shader.numRenderPasses();
            case C_NUM_CHARS: return (int)shader.info().numChars;
            case C_HAS_SOUND: return shader.info().hasSound;
            case C_USE_TEX: return shader.info().usesTextures;
            case C_USE_MUSIC: return shader.info().usesMusic;
            case C_USE_KEYBOARD: return shader.info().usesKeyboard;
            case C_USE_MOUSE: return shader.info().usesMouse;
            case C_FLAGS: return shader.info().flags;
            case C_IMAGE: return QVariant();
        }
    }

    if (role == Qt::DecorationRole)
    {
        if (column.id == C_IMAGE)
        {
            auto i = p_->pixMap.find(shader.info().id);
            if (i != p_->pixMap.end())
                return i.value();
            auto img = p_->api->getSnapshot(shader.info().id, false);
            if (img.isNull())
                return QVariant();
            auto p = QPixmap::fromImage(
                        img.scaled(48, 48, Qt::IgnoreAspectRatio,
                                           Qt::SmoothTransformation) );
            p_->pixMap.insert(shader.info().id, p);
            return p;
        }
    }

    return QVariant();
}

QVariant ShaderListModel::headerData(
        int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole
            && section >= 0 && section < p_->columns.size())
    {
        return p_->columns[section].name;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}
