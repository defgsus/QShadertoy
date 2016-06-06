/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 6/6/2016</p>
*/

#include <algorithm>

#include <QLayout>
#include <QPainter>
#include <QComboBox>
#include <QAbstractItemModel>
#include <QVector>
#include <QDateTime>

#include "tableplotview.h"
#include "log.h"

struct TablePlotView::Private
{
    Private(TablePlotView* p)
        : p         (p)
        , model     (nullptr)
    { }

    void createWidgets();
    void connectModel();
    void updateCombos();
    void updateFromModel();
    void freeValues();
    void getData(int column1, int column2);
    double toDouble(const QVariant&) const;
    void renderPlot(QPainter& p, const QRect& rect);

    TablePlotView* p;
    QAbstractItemModel* model;
    QList<QMetaObject::Connection> cons;

    struct Value
    {
        double x, y;
        QVariant dataX, dataY;
        QModelIndex srcX, srcY;
    };

    QVector<Value*> values;

    QComboBox *cbX, *cbY;
    QWidget *plotWidget;
};

TablePlotView::TablePlotView(QWidget *parent)
    : QWidget   (parent)
    , p_        (new Private(this))
{
    p_->createWidgets();
}

TablePlotView::~TablePlotView()
{
    p_->freeValues();
    delete p_;
}

void TablePlotView::Private::createWidgets()
{
    auto lv = new QVBoxLayout(p);
    lv->setMargin(0);

        cbY = new QComboBox(p);
        connect(cbY, static_cast<void(QComboBox::*)(int)>
                        (&QComboBox::currentIndexChanged),
                            [=](){ updateFromModel(); });
        lv->addWidget(cbY);

        plotWidget = new QWidget(p);
        plotWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        lv->addWidget(plotWidget);

        cbX = new QComboBox(p);
        connect(cbX, static_cast<void(QComboBox::*)(int)>
                        (&QComboBox::currentIndexChanged),
                            [=](){ updateFromModel(); });
        lv->addWidget(cbX);
}



void TablePlotView::setModel(QAbstractItemModel* m)
{
    p_->model = m;
    p_->connectModel();
    p_->updateCombos();
    p_->updateFromModel();
}

void TablePlotView::Private::connectModel()
{
    for (auto c : cons)
        disconnect(c);
    cons.clear();

    cons << connect(model, &QAbstractItemModel::dataChanged,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::modelReset,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::columnsInserted,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::columnsMoved,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::columnsRemoved,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::rowsInserted,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::rowsMoved,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::rowsRemoved,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::headerDataChanged,
                    [=](){ updateFromModel(); });
    cons << connect(model, &QAbstractItemModel::layoutChanged,
                    [=](){ updateFromModel(); });
}

void TablePlotView::Private::updateCombos()
{
    cbX->clear();
    cbY->clear();
    if (!model)
        return;

    int num = model->columnCount();
    for (int i=0; i<num; ++i)
    {
        auto n = model->headerData(i, Qt::Horizontal).toString();
        cbX->addItem(n);
        cbY->addItem(n);
    }
}

void TablePlotView::Private::updateFromModel()
{
    getData(cbX->currentIndex(), cbY->currentIndex());

    p->update();
}

void TablePlotView::Private::freeValues()
{
    for (auto v : values)
        delete v;
    values.clear();
}

void TablePlotView::Private::getData(int column1, int column2)
{
    freeValues();

    if (!model)
        return;

    if (column1 < 0 || column1 >= model->columnCount()
        || column2 < 0 || column2 >= model->columnCount())
        return;

    int num = model->rowCount();
    for (int i=0; i<num; ++i)
    {
        auto v = new Value;
        v->srcX = model->index(i, column1);
        v->srcY = model->index(i, column2);
        v->dataX = model->data( v->srcX );
        v->dataY = model->data( v->srcY );
        v->x = toDouble(v->dataX);
        v->y = toDouble(v->dataY);

        values.push_back(v);
    }

    // get min/max
    if (!values.empty())
    {
        double miX = values.front()->x, maX = miX,
               miY = values.front()->y, maY = miY;
        for (auto v : values)
            miX = std::min(miX, v->x), maX = std::max(maX, v->x),
            miY = std::min(miY, v->y), maY = std::max(maY, v->y);

        // normalize
        double mulX = 1., mulY = 1.;
        if (maX > miX)
            mulX = 1. / (maX - miX);
        if (maY > miY)
            mulY = 1. / (maY - miY);
        for (auto v : values)
            v->x = (v->x - miX) * mulX,
            v->y = (v->y - miY) * mulY;
    }

    // sort on x
    std::sort(values.begin(), values.end(), [](const Value* l, const Value* r)
    {
        return l->x < r->x;
    });
}

double TablePlotView::Private::toDouble(const QVariant& v) const
{
    double val;
    switch (QMetaType::Type(v.type()))
    {
        case QMetaType::Bool: val = v.toBool() ? 1. : 0.; break;
        case QMetaType::QString:
        {
            auto s = v.toString();
            val = s.isEmpty() ? 0.
                : (double)((unsigned char)(s[0].toLatin1()));
        }
        break;
        case QMetaType::QDateTime:
            val = v.toDateTime().toMSecsSinceEpoch(); break;
        case QMetaType::QDate:
            val = v.toDate().toJulianDay(); break;
        default: val = v.toDouble();
    }
    return val;
}

void TablePlotView::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    p_->renderPlot(p, p_->plotWidget->geometry());
}

void TablePlotView::Private::renderPlot(QPainter& p, const QRect& rect)
{
    p.setClipRect(rect);
    // background
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(QColor(40,40,40)));
    p.drawRect(rect);

    p.setRenderHint(QPainter::Antialiasing, true);

    // grid

    // -- data --

    p.setBrush(QBrush(Qt::white));
    p.setPen(QPen(Qt::white));
    double lx=0., ly=0.;
    for (auto v : values)
    {
        double
            x =                     v->x * rect.width()  + rect.left(),
            y = rect.height() - 1 - v->y * rect.height() + rect.top();
        //ST_INFO(minX << " " << maxX << " "
        //        << mulX << " " << mulY << " " << x << " " << y);
        p.drawEllipse(QPointF(x, y), 2.5, 2.5);
        if (v != values.front())
            p.drawLine(QPointF(lx,ly), QPointF(x,y));
        lx = x, ly = y;
    }
}

