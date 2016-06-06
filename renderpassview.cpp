/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#include <QLayout>
#include <QPlainTextEdit>
#include <QTabBar>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QFont>
#include <QJsonDocument>
#include <QImage>
#include <QPixmap>

#include "renderpassview.h"
#include "shadertoyshader.h"
#include "shadertoyapi.h"

struct RenderPassView::Private
{
    Private(RenderPassView*p)
        : p     (p)
        , api   (new ShadertoyApi(p))
    {
        connect(api, SIGNAL(textureReceived(QString,QImage)),
                p, SLOT(p_onTexture_(QString,QImage)),
                Qt::QueuedConnection);
    }

    struct InputWidget
    {
        QLabel *desc, *imgLabel;
        QComboBox *interpol, *wrap;
        QCheckBox *vFlip;
        QString src;
    };

    void createWidgets();
    void selectTab(int idx);

    RenderPassView* p;
    ShadertoyShader shader;

    ShadertoyApi* api;

    QTabBar* tabBar;
    QPlainTextEdit* textEdit, *jsonView;
    QVector<InputWidget> inputWidgets;
};

RenderPassView::RenderPassView(QWidget *parent)
    : QWidget   (parent)
    , p_        (new Private(this))
{
    p_->createWidgets();
}

RenderPassView::~RenderPassView()
{
    delete p_;
}

void RenderPassView::Private::createWidgets()
{
    auto lv = new QVBoxLayout(p);
    lv->setMargin(0);

    tabBar = new QTabBar(p);
    lv->addWidget(tabBar);
    connect(tabBar, &QTabBar::currentChanged, [=](int idx)
    {
        selectTab(idx);
    });

    textEdit = new QPlainTextEdit(p);
    textEdit->setReadOnly(true);
    lv->addWidget(textEdit);

    // XXX does not work for Mac
    QFont f("Monospace");
    f.setStyleHint(QFont::Monospace);
    textEdit->setFont(f);
    textEdit->setTabStopWidth(QFontMetrics(f).width("    "));

    auto lh = new QHBoxLayout();
    lv->addLayout(lh);

        for (int i=0; i<4; ++i)
        {
            auto lv = new QVBoxLayout();
            lh->addLayout(lv);

            InputWidget iw;
            iw.desc = new QLabel(p);
            lv->addWidget(iw.desc);

            iw.imgLabel = new QLabel(p);
            lv->addWidget(iw.imgLabel);

            iw.interpol = new QComboBox(p);
            iw.interpol->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::F_NEAREST),
                        (int)ShadertoyInput::F_NEAREST);
            iw.interpol->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::F_LINEAR),
                        (int)ShadertoyInput::F_LINEAR);
            lv->addWidget(iw.interpol);

            iw.wrap = new QComboBox(p);
            iw.wrap->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::W_CLAMP),
                        (int)ShadertoyInput::W_CLAMP);
            iw.wrap->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::W_REPEAT),
                        (int)ShadertoyInput::W_REPEAT);
            lv->addWidget(iw.wrap);

            iw.vFlip = new QCheckBox(tr("v-flip"), p);
            lv->addWidget(iw.vFlip);

            inputWidgets.push_back(iw);
        }

    jsonView = new QPlainTextEdit(p);
    jsonView->setReadOnly(true);
    lv->addWidget(jsonView);
}

const ShadertoyShader& RenderPassView::shader() const { return p_->shader; }
void RenderPassView::setShader(const ShadertoyShader& s)
{
    p_->shader = s;

    while (p_->tabBar->count())
        p_->tabBar->removeTab(0);

    for (size_t i=0; i<p_->shader.numRenderPasses(); ++i)
    {
        auto& rp = p_->shader.renderPass(i);
        p_->tabBar->addTab(rp.name());
    }

    p_->tabBar->setCurrentIndex(0);
    p_->selectTab(0);
}

void RenderPassView::Private::selectTab(int idx)
{
    if (idx < 0 || size_t(idx) >= shader.numRenderPasses())
    {
        textEdit->clear();
        jsonView->clear();
        for (size_t i=0; i<4; ++i)
        {
            InputWidget& iw = inputWidgets[i];
            iw.desc->setText("-");
            iw.interpol->setEnabled(false);
            iw.wrap->setEnabled(false);
            iw.vFlip->setEnabled(false);
        }
        return;
    }

    auto& rp = shader.renderPass(idx);
    textEdit->setPlainText(rp.fragmentSource());
    jsonView->setPlainText(
                QString::fromUtf8(QJsonDocument(rp.jsonData()).toJson()));

    for (size_t i=0; i<4; ++i)
    {
        InputWidget& iw = inputWidgets[i];

        iw.imgLabel->clear();
        iw.src.clear();

        if (i >= rp.numInputs())
        {
            iw.desc->setText("-");
            iw.interpol->setEnabled(false);
            iw.wrap->setEnabled(false);
            iw.vFlip->setEnabled(false);
        }
        else
        {
            const ShadertoyInput& inp = rp.input(i);

            iw.interpol->setEnabled(true);
            iw.wrap->setEnabled(true);
            iw.vFlip->setEnabled(true);

            iw.desc->setText(inp.typeName());
            iw.vFlip->setChecked(inp.vFlip);

            iw.interpol->setCurrentText(inp.filterTypeName());
            iw.wrap->setCurrentText(inp.wrapModeName());

            iw.src = inp.src;
            if (inp.type == ShadertoyInput::T_TEXTURE
              || inp.type == ShadertoyInput::T_CUBEMAP
              || inp.type == ShadertoyInput::T_BUFFER)
            {
                api->getTexture(inp.src);
            }
        }
    }
}

void RenderPassView::p_onTexture_(const QString &src, const QImage &img)
{
    /*
    int idx = p_->tabBar->currentIndex();
    if (idx < 0 || idx > 4)
        return;
    */

    for (size_t i=0; i<4; ++i)
    {
        Private::InputWidget& iw = p_->inputWidgets[i];
        if (iw.src == src)
            iw.imgLabel->setPixmap(QPixmap::fromImage(
                                       img.scaled(128,128)));
    }
}
