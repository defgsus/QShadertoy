/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#ifndef TABLEPLOTVIEW_H
#define TABLEPLOTVIEW_H

#include <QWidget>
class QAbstractItemModel;

/** Generic view to plot values from
    QAbstractTableView like models against each other */
class TablePlotView : public QWidget
{
    Q_OBJECT
public:
    explicit TablePlotView(QWidget *parent = 0);
    ~TablePlotView();

    QAbstractItemModel* model();

signals:

public slots:

    /** No ownership is taken */
    void setModel(QAbstractItemModel*);

protected:

    void paintEvent(QPaintEvent*) override;

private:
    struct Private;
    Private* p_;
};

#endif // TABLEPLOTVIEW_H
