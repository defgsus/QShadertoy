/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include <QList>
#include <QColor>

#include "shaderlistmodel.h"
#include "shadertoyapi.h"
#include "shadertoyshader.h"
#include "log.h"

struct ShaderListModel::Private
{
    Private(ShaderListModel* p)
        : p     (p)
        , api   (new ShadertoyApi(p))
    {
        api->loadShaderList();
        connect(api, &ShadertoyApi::shaderListChanged, [=]()
        {
            copyListFromApi();
        });
    }

    void copyListFromApi();

    ShaderListModel* p;
    ShadertoyApi* api;
    QList<ShadertoyShader> shaders;
    QStringList headerNames;
};

ShaderListModel::ShaderListModel(QObject *parent)
    : QAbstractTableModel   (parent)
    , p_        (new Private(this))
{
    p_->headerNames
            << tr("id")
            << tr("name")
            << tr("user")
            << tr("date")
            << tr("views")
            << tr("likes")
            << tr("passes")
            << tr("chars")
            << tr("use texture")
            << tr("use music")
            << tr("flags")
               ;
}

ShaderListModel::~ShaderListModel()
{
    delete p_;
}

ShadertoyApi* ShaderListModel::api() { return p_->api; }

const QStringList& ShaderListModel::shaderIds() const { return p_->api->shaderIds(); }

void ShaderListModel::Private::copyListFromApi()
{
    ST_DEBUG2("ShaderListModel::copyListFromApi()");

    p->beginResetModel();

    shaders.clear();
    for (auto id : api->shaderIds())
        shaders << api->getShader(id);

    p->endResetModel();
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
    return parent.isValid() ? 0 : p_->headerNames.size();
}

int ShaderListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : p_->shaders.size();
}

QVariant ShaderListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= p_->shaders.size())
        return QVariant();

    const ShadertoyShader shader = p_->shaders[index.row()];

    if (!shader.isValid())
    {
        if (role == Qt::BackgroundColorRole)
            return QColor(255,128,128);
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            if (index.column() == C_ID)
                return p_->shaders[index.row()].info().id;
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch ((Columns)index.column())
        {
            case C_ID: return shader.info().id;
            case C_NAME: return shader.info().name;
            case C_USER: return shader.info().username;
            case C_DATE: return shader.info().date;
            case C_VIEWS: return shader.info().views;
            case C_LIKES: return shader.info().likes;
            case C_PASSES: return (int)shader.numRenderPasses();
            case C_NUM_CHARS: return (int)shader.info().numChars;
            case C_USE_TEX: return shader.info().usesTextures;
            case C_USE_MUSIC: return shader.info().usesMusic;
            case C_FLAGS: return shader.info().flags;
        }
    }

    return QVariant();
}

QVariant ShaderListModel::headerData(
        int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole
            && section >= 0 && section < p_->headerNames.size())
    {
        return p_->headerNames[section];
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}
