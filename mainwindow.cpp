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

#include "mainwindow.h"
#include "shadertoyapi.h"
#include "shadertoyshader.h"
#include "shaderlistmodel.h"
#include "shadersortmodel.h"
#include "renderpassview.h"
#include "shadertoyrenderwidget.h"
#include "tableplotview.h"
#include "settings.h"
#include "logview.h"

struct MainWindow::Private
{
    Private(MainWindow* p)
        : win       (p)
        , api       (new ShadertoyApi(win))
    { }

    void createWidgets();
    void createMenu();
    void downloadUnknownShaders();
    void loadShaders();

    void setShader(const QModelIndex&);
    void setShader(const ShadertoyShader&);

    MainWindow* win;

    ShadertoyApi* api;
    ShaderListModel* shaderList;
    ShaderSortModel* shaderSortModel;

    int curDownShader;

    ShadertoyRenderWidget* renderWidget;
    QLineEdit* tableFilterEdit;
    QTableView* shaderTable;
    RenderPassView* passView;
    TablePlotView* plotView;
    QLabel * infoLabel;

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
    p_->loadShaders();

    p_->setShader(ShadertoyShader());

    connect(p_->api, SIGNAL(shaderListReceived()),
            this, SLOT(p_onShaderList_()), Qt::QueuedConnection);
    connect(p_->api, SIGNAL(shaderReceived()),
            this, SLOT(p_onShader_()), Qt::QueuedConnection);

}

MainWindow::~MainWindow()
{
    Settings::instance().storeGeometry(this);

    delete p_;
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
                auto lh1 = new QHBoxLayout();
                lv1->addLayout(lh1);

                    auto but = new QToolButton(win);
                    but->setText("|<");
                    lh1->addWidget(but);
                    connect(but, &QToolButton::clicked, [=]()
                    {
                        renderWidget->rewind();
                    });

                    but = new QToolButton(win);
                    but->setText(">");
                    lh1->addWidget(but);
                    connect(but, &QToolButton::clicked, [=]()
                    {
                        renderWidget->setPlaying(true);
                    });

                    but = new QToolButton(win);
                    but->setText("#");
                    lh1->addWidget(but);
                    connect(but, &QToolButton::clicked, [=]()
                    {
                        renderWidget->setPlaying(false);
                    });

                    lh1->addStretch();

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

                connect(shaderTable, &QTableView::doubleClicked,
                        [=](const QModelIndex& idx){ setShader(idx); });

            lv1 = new QVBoxLayout();
            lh->addLayout(lv1);

                auto tab = new QTabWidget(win);
                lv1->addWidget(tab);

                    passView = new RenderPassView(win);
                    tab->addTab(passView, "source");

                    plotView = new TablePlotView(win);
                    plotView->setModel(shaderList);
                    tab->addTab(plotView, "plot");


                auto logView = new LogView(win);
                lv1->addWidget(logView);

        infoLabel = new QLabel();
        lv->addWidget(infoLabel);
}

void MainWindow::Private::createMenu()
{
    auto menu = win->menuBar()->addMenu(tr("Shaders"));

    auto a = menu->addAction(tr("Download shader list"));
    connect(a, &QAction::triggered, [=](){ api->downloadShaderList(); });

    a = menu->addAction(tr("Download shaders"));
    connect(a, &QAction::triggered, [=](){ downloadUnknownShaders(); });

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

    str /*<< "id: " << s.info().id
        << "\nname: " << s.info().name
        << "\nuser: " << s.info().username
        << "\nviews: " << s.info().views
        << "\nlikes: " << s.info().likes
        << "\nflags: " << s.info().flags
           */
        << "\ndesc: " << s.info().description
    ;
    infoLabel->setText(text);

    passView->setShader(s);
    renderWidget->setShader(s);
    renderWidget->update();
}

void MainWindow::p_onShaderList_()
{
    const auto ids = p_->api->shaderIds();
    p_->shaderList->initWithIds(ids);
}

void MainWindow::p_onShader_()
{
    auto s = p_->api->shader();
    p_->shaderList->setShader(s.info().id, s);

    // download next shader from list
    const auto ids = p_->shaderList->shaderIds();
    while (p_->curDownShader < ids.size()
           && p_->shaderList->getShader(p_->curDownShader).isValid())
        ++p_->curDownShader;
    if (p_->curDownShader < ids.size())
        p_->api->downloadShader(ids[p_->curDownShader]);

}

void MainWindow::Private::downloadUnknownShaders()
{
    const auto ids = shaderList->shaderIds();
    curDownShader = 0;
    while (curDownShader < ids.size()
           && shaderList->getShader(curDownShader).isValid())
        ++curDownShader;
    if (curDownShader < ids.size())
        api->downloadShader(ids[curDownShader]);
}

void MainWindow::Private::loadShaders()
{
    shaderList->loadShaders();
}
