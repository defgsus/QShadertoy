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

#include "MainWindow.h"
#include "core/ShadertoyApi.h"
#include "core/ShadertoyShader.h"
#include "core/ShaderListModel.h"
#include "core/ShaderSortModel.h"
#include "core/ShadertoyOffscreenRenderer.h"
#include "RenderpassView.h"
#include "ShadertoyRenderWidget.h"
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
    QLabel * infoLabel;
    QProgressBar* progressBar;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow   (parent)
    , p_            (new Private(this))
{
    setObjectName("MainWindow");
    setMinimumSize(640, 480);
    Settings::instance().restoreGeometry(this);

    p_->createWidgets();
    p_->createMenu();

    //p_->api->getShaderList();
    //p_->downloadAllShaders();
    //p_->loadShaders();

    p_->setShader(ShadertoyShader());

    /*
    connect(p_->api, SIGNAL(shaderListReceived()),
            this, SLOT(p_onShaderList_()), Qt::QueuedConnection);
    connect(p_->api, SIGNAL(shaderReceived(QString)),
            this, SLOT(p_onShader_(QString)), Qt::QueuedConnection);
    */
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

void MainWindow::Private::createWidgets()
{
    win->setCentralWidget(new QWidget(win));
    auto lv = new QVBoxLayout(win->centralWidget());

        auto lh = new QHBoxLayout();
        lv->addLayout(lh);

            auto lv1 = new QVBoxLayout();
            lh->addLayout(lv1);

                // render widget
                renderWidget = new ShadertoyRenderWidget(win);
                lv1->addWidget(renderWidget);

                // playbar
                lv1->addWidget(renderWidget->createPlaybar(win));

                // shader list
                tableFilterEdit = new QLineEdit(win);
                lv1->addWidget(tableFilterEdit);
                connect(tableFilterEdit, &QLineEdit::textChanged, [=]()
                {
                    shaderSortModel->setFulltextFilter(
                                tableFilterEdit->text());
                });

                shaderTable = new QTableView(win);
                shaderTable->setSortingEnabled(true);
                lv1->addWidget(shaderTable);
                shaderSortModel = new ShaderSortModel(shaderTable);
                shaderSortModel->setFilterRole(Qt::DisplayRole);

                shaderList = new ShaderListModel(shaderTable);
                shaderSortModel->setSourceModel(shaderList);
                shaderTable->setModel(shaderSortModel);

                connect(shaderTable, &QTableView::activated,
                        [=](const QModelIndex& idx){ setShader(idx); });

            lv1 = new QVBoxLayout();
            lh->addLayout(lv1, 3);

                auto tab = new QTabWidget(win);
                tab->setSizePolicy(
                            QSizePolicy::Expanding, QSizePolicy::Expanding);
                lv1->addWidget(tab);

                    passView = new RenderPassView(win);
                    connect(passView, &RenderPassView::shaderChanged,
                            [=](){ onShaderEdited(); });
                    tab->addTab(passView, "source");

                    plotView = new TablePlotView(win);
                    plotView->setModel(shaderList);
                    tab->addTab(plotView, "plot");

#if 0
                auto logView = new LogView(win);
                lv1->addWidget(logView);
#endif

        infoLabel = new QLabel();
        lv->addWidget(infoLabel);

        progressBar = new QProgressBar(win);
        progressBar->setVisible(false);
        lv->addWidget(progressBar);
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

#ifndef NDEBUG
    menu = win->menuBar()->addMenu(tr("Debug"));

    a = menu->addAction(tr("Test offscreen render"));
    connect(a, &QAction::triggered, [=]()
    {
        ShadertoyOffscreenRenderer r(win);
        r.setShader( passView->shader() );
        auto img = r.renderToImage(QSize(256, 256));
        infoLabel->setPixmap(QPixmap::fromImage(img));
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

    str << "name: " << s.info().name
        /*<< "id: " << s.info().id
        << "\nuser: " << s.info().username
        << "\nviews: " << s.info().views
        << "\nlikes: " << s.info().likes
        << "\nflags: " << s.info().flags
           */
        << " desc: " << s.info().description
    ;
    infoLabel->setText(text);

    passView->setShader(s);
    renderWidget->setShader(s);
    renderWidget->update();
}


void MainWindow::Private::onShaderEdited()
{
    renderWidget->setShader(passView->shader());
}
