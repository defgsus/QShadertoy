/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include "ShaderSortModel.h"
#include "ShaderListModel.h"
#include "ShadertoyShader.h"

ShaderSortModel::ShaderSortModel(QObject *parent)
    : QSortFilterProxyModel (parent)
{

}


void ShaderSortModel::setFulltextFilter(const QString &s)
{
    p_fulltextFilters_.clear();
    auto list = s.split(";");
    for (QString s : list)
    {
        s = s.simplified();
        if (!s.isEmpty())
            p_fulltextFilters_ << s;
    }

    invalidate();
}

bool ShaderSortModel::filterAcceptsRow(
        int source_row, const QModelIndex &source_parent) const
{
    if (!QSortFilterProxyModel::filterAcceptsRow(
                source_row, source_parent))
        return false;

    if (p_fulltextFilters_.isEmpty())
        return true;

    auto srcModel = qobject_cast<ShaderListModel*>(sourceModel());
    if (!srcModel)
        return false;

    if (source_row < 0
     || source_row >= srcModel->rowCount(QModelIndex()))
        return false;

    const ShadertoyShader& shader = srcModel->getShader(source_row);

    for (const auto & s : p_fulltextFilters_)
        if (shader.containsString(s))
            return true;

    return false;
}
