/** @file mainwindow.cpp

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#include <QJsonDocument>
#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QTextStream>
#include <QTableView>
#include <QLineEdit>
#include <QToolButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QProgressBar>
#include <QDockWidget>
#include <QStatusBar>

#include "MainWindow.h"
#include "core/ShadertoyApi.h"
#include "core/ShadertoyShader.h"
#include "core/ShaderListModel.h"
#include "core/ShaderSortModel.h"
#include "core/ShadertoyOffscreenRenderer.h"
#include "RenderpassView.h"
#include "ShadertoyRenderWidget.h"
#include "ShaderInfoView.h"
#include "TablePlotView.h"
#include "Settings.h"
#include "LogView.h"

struct MainWindow::Private
{
    Private(MainWindow* p)
        : win       (p)
    { }

    void createWidgets();
    void createMenu();
    void createDockWidget(const QString& title, QWidget* w);

    void loadShaders();

    void setShader(const QModelIndex&);
    void setShader(const ShadertoyShader&);

    void onShaderEdited();

    MainWindow* win;

    ShaderListModel* shaderList;
    ShaderSortModel* shaderSortModel;

    int curDownShader;

    ShadertoyRenderWidget* renderWidget;
    QLineEdit* tableFilterEdit;
    QTableView* shaderTable;
    RenderPassView* passView;
    TablePlotView* plotView;
    ShaderInfoView * infoView;
    QProgressBar* progressBar;

    QMenu* viewMenu;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow   (parent)
    , p_            (new Private(this))
{
    setObjectName("MainWindow");
    setMinimumSize(640, 480);

    p_->createMenu();
    p_->createWidgets();

    Settings::instance().restoreGeometry(this);

    p_->setShader(ShadertoyShader());
}

MainWindow::~MainWindow()
{
    Settings::instance().storeGeometry(this);

    delete p_;
}

void MainWindow::showEvent(QShowEvent* )
{
    p_->shaderList->api()->loadAllShaders();
}

void MainWindow::Private::createDockWidget(
        const QString &title, QWidget *w)
{
    auto dock = new QDockWidget(title, win);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setWidget(w);
    dock->setObjectName(w->objectName());
    win->addDockWidget(Qt::AllDockWidgetAreas, dock);
    viewMenu->addAction(dock->toggleViewAction());
}

void MainWindow::Private::createWidgets()
{
    // render widget
    auto w = new QWidget(win);
    auto lv = new QVBoxLayout(w);

        renderWidget = new ShadertoyRenderWidget(w);
        lv->addWidget(renderWidget, 2);

        // playbar
        auto tb = renderWidget->createPlaybar(w);
        lv->addWidget(tb);

    w->setObjectName("RenderWidget");
    createDockWidget(tr("render view"), w);

    // shader list
    w = new QWidget(win);
    lv = new QVBoxLayout(w);

        tableFilterEdit = new QLineEdit(win);
        lv->addWidget(tableFilterEdit);
        connect(tableFilterEdit, &QLineEdit::textChanged, [=]()
        {
            shaderSortModel->setFulltextFilter(
                        tableFilterEdit->text());
        });

        shaderTable = new QTableView(win);
        lv->addWidget(shaderTable, 2);
        shaderTable->setSortingEnabled(true);
        shaderSortModel = new ShaderSortModel(shaderTable);
        shaderSortModel->setFilterRole(Qt::DisplayRole);

        shaderList = new ShaderListModel(shaderTable);
        shaderSortModel->setSourceModel(shaderList);
        shaderTable->setModel(shaderSortModel);

        connect(shaderTable, &QTableView::activated,
                [=](const QModelIndex& idx){ setShader(idx); });

    w->setObjectName("ShaderList");
    createDockWidget(tr("shader list"), w);

    // source / inputs
    passView = new RenderPassView(win);
    connect(passView, &RenderPassView::shaderChanged,
            [=](){ onShaderEdited(); });
    passView->setObjectName("PassView");
    createDockWidget(tr("source"), passView);

    // comparison plot
    plotView = new TablePlotView(win);
    plotView->setModel(shaderList);
    plotView->setObjectName("PlotView");
    createDockWidget(tr("shader compare plot"), plotView);

#if 0
    auto logView = new LogView(win);
    lv1->addWidget(logView);
#endif

    // info view
    infoView = new ShaderInfoView(win);
    infoView->setObjectName("InfoView");
    createDockWidget(tr("shader info"), infoView);

    // progress bar
    progressBar = new QProgressBar(win);
    progressBar->setVisible(false);
    win->statusBar()->addPermanentWidget(progressBar);
    connect(shaderList->api(), &ShadertoyApi::downloadProgress, [=](int p)
    {
        progressBar->setValue(p);
    });
    connect(shaderList->api(), &ShadertoyApi::mergeFinished, [=]()
    {
        progressBar->setVisible(false);
    });
}

void MainWindow::Private::createMenu()
{
    // ########### SHADERS ############

    auto menu = win->menuBar()->addMenu(tr("Shaders"));

    auto a = menu->addAction(tr("Load shaders from disk"));
    connect(a, &QAction::triggered, [=]()
        { shaderList->api()->loadAllShaders(); });

    menu->addSeparator();

    a = menu->addAction(tr("Merge with shadertoy.com"));
    connect(a, &QAction::triggered, [=]()
    {
        progressBar->setValue(0);
        progressBar->setVisible(true);
        shaderList->api()->mergeWithWeb();
    });

    a = menu->addAction(tr("Stop web request"));
    connect(a, &QAction::triggered, [=](){ shaderList->api()->stopRequests(); });


    // ########## Options ############
    menu = win->menuBar()->addMenu(tr("Options"));

    a = menu->addAction(tr("Display thumbnails"));
    a->setCheckable(true);
    connect(a, &QAction::triggered, [=](bool e)
    {
        shaderList->setEnableThumbnails(e);
    });


    // ########## VIEW ############
    viewMenu = win->menuBar()->addMenu(tr("View"));


    // ############ DEBUG ##############
#ifndef NDEBUG
    menu = win->menuBar()->addMenu(tr("Debug"));

    a = menu->addAction(tr("Test offscreen render"));
    connect(a, &QAction::triggered, [=]()
    {
        ShadertoyOffscreenRenderer r(win);
        r.setShader( passView->shader() );
        auto img = r.renderToImage(QSize(256, 256));
        //someLabel->setPixmap(QPixmap::fromImage(img));
    });

    a = menu->addAction(tr("create snapshots"));
    connect(a, &QAction::triggered, [=]()
    {
        auto ids = shaderList->shaderIds();
        for (auto& id : ids)
        {
            qDebug() << "rendering" << id;
            QImage img = shaderList->api()->getSnapshot(id, true);
            qDebug() << (img.isNull() ? "FAILED" : "ok");
        }
    });

    a = menu->addAction(tr("dump window state"));
    connect(a, &QAction::triggered, [=]()
    {
        QByteArray a = win->saveState();
        qDebug() << a.toPercentEncoding();
    });

#endif

}

void MainWindow::Private::setShader(const QModelIndex& fidx)
{
    auto idx = shaderSortModel->mapToSource(fidx);
    setShader(shaderList->getShader(idx));
}

void MainWindow::Private::setShader(const ShadertoyShader& s)
{
    QString text;
    QTextStream str(&text);

    infoView->setShader(s);
    passView->setShader(s);
    renderWidget->setShader(s);
    renderWidget->update();
}


void MainWindow::Private::onShaderEdited()
{
    renderWidget->setShader(passView->shader());
}
