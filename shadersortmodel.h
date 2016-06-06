/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef SHADERSORTMODEL_H
#define SHADERSORTMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

class ShaderSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ShaderSortModel(QObject *parent = 0);

    void setFulltextFilter(const QString&);

protected:

    bool filterAcceptsRow(
            int source_row, const QModelIndex &source_parent) const override;

private:

    QStringList p_fulltextFilters_;
};

#endif // SHADERSORTMODEL_H
