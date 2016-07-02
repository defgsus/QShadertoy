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
#include <QToolButton>
#include <QFont>
#include <QJsonDocument>
#include <QImage>
#include <QPixmap>
#include <QMap>

#include "renderpassview.h"
#include "shadertoyshader.h"
#include "shadertoyapi.h"
#include "log.h"

struct RenderPassView::Private
{
    Private(RenderPassView*p)
        : p             (p)
        , api           (new ShadertoyApi(p))
        , ignoreChange  (false)
    {
        connect(api, SIGNAL(textureReceived(QString,QImage)),
                p, SLOT(p_onTexture_(QString,QImage)),
                Qt::QueuedConnection);
    }

    struct InputWidget
    {
        QLabel *desc, *imgLabel;
        QComboBox *filter, *wrap;
        QCheckBox *vFlip;
        QString src;
    };

    void createWidgets();
    void selectTab(int idx);
    void sourceEdited();
    void inputsEdited();
    QPixmap getPixmap(const QString& src);

    RenderPassView* p;
    ShadertoyShader shader;

    ShadertoyApi* api;

    bool ignoreChange;

    QTabBar* tabBar;
    QPlainTextEdit* textEdit, *jsonView;
    QVector<InputWidget> inputWidgets;
    QMap<QString, QPixmap> pixmaps;
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

            iw.filter = new QComboBox(p);
            iw.filter->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::F_NEAREST),
                        (int)ShadertoyInput::F_NEAREST);
            iw.filter->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::F_LINEAR),
                        (int)ShadertoyInput::F_LINEAR);
            iw.filter->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::F_MIPMAP),
                        (int)ShadertoyInput::F_MIPMAP);
            connect(iw.filter, static_cast<void(QComboBox::*)(int)>
                    (&QComboBox::currentIndexChanged),
                        [=](){ if (!ignoreChange) inputsEdited(); });
            lv->addWidget(iw.filter);

            iw.wrap = new QComboBox(p);
            iw.wrap->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::W_CLAMP),
                        (int)ShadertoyInput::W_CLAMP);
            iw.wrap->addItem(
                        ShadertoyInput::nameForType(ShadertoyInput::W_REPEAT),
                        (int)ShadertoyInput::W_REPEAT);
            connect(iw.wrap, static_cast<void(QComboBox::*)(int)>
                    (&QComboBox::currentIndexChanged),
                        [=](){ if (!ignoreChange) inputsEdited(); });
            lv->addWidget(iw.wrap);

            iw.vFlip = new QCheckBox(tr("v-flip"), p);
            connect(iw.vFlip, &QCheckBox::toggled, [=]()
                { if (!ignoreChange) inputsEdited(); });
            lv->addWidget(iw.vFlip);

            inputWidgets.push_back(iw);
        }

    auto but = new QToolButton(p);
    but->setText(">");
    but->setShortcut(Qt::ALT + Qt::Key_Return);
    connect(but, &QToolButton::clicked, [=]() { sourceEdited(); });
    lv->addWidget(but);

    jsonView = new QPlainTextEdit(p);
    jsonView->setReadOnly(true);
    lv->addWidget(jsonView);
    //jsonView->setVisible(false);
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

        // preload textures
        for (size_t j=0; j<rp.numInputs(); ++j)
        {
            const ShadertoyInput& inp = rp.input(j);
            if (inp.type == ShadertoyInput::T_TEXTURE
              || inp.type == ShadertoyInput::T_CUBEMAP
              || inp.type == ShadertoyInput::T_BUFFER)
            {
                p_->getPixmap(inp.src);
            }
        }

    }

    p_->tabBar->setCurrentIndex(0);
    p_->selectTab(0);
}

void RenderPassView::Private::selectTab(int idx)
{
    ignoreChange = true;

    if (idx < 0 || size_t(idx) >= shader.numRenderPasses())
    {
        textEdit->clear();
        jsonView->clear();
        for (size_t i=0; i<4; ++i)
        {
            InputWidget& iw = inputWidgets[i];
            iw.desc->setText("-");
            iw.filter->setEnabled(false);
            iw.wrap->setEnabled(false);
            iw.vFlip->setEnabled(false);
        }
        ignoreChange = false;
        return;
    }

    auto rp = shader.renderPass(idx);
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
            iw.filter->setEnabled(false);
            iw.wrap->setEnabled(false);
            iw.vFlip->setEnabled(false);
        }
        else
        {
            const ShadertoyInput& inp = rp.input(i);

            iw.filter->setEnabled(true);
            iw.wrap->setEnabled(true);
            iw.vFlip->setEnabled(true);

            iw.desc->setText(inp.typeName());
            iw.vFlip->setChecked(inp.vFlip);

            iw.filter->setCurrentText(inp.filterTypeName());
            iw.wrap->setCurrentText(inp.wrapModeName());

            iw.src = inp.src;
            if (inp.type == ShadertoyInput::T_TEXTURE
              || inp.type == ShadertoyInput::T_CUBEMAP
              || inp.type == ShadertoyInput::T_BUFFER)
            {
                iw.imgLabel->setPixmap( getPixmap(inp.src) );
            }
        }
    }
    ignoreChange = false;
}

QPixmap RenderPassView::Private::getPixmap(const QString& src)
{
    ST_DEBUG2("RenderPassView::getPixmap(" << src << ") "
              "present=" << pixmaps.contains(src));

    if (pixmaps.contains(src))
        return pixmaps[src];

    api->getTexture(src);
    return QPixmap(64, 64);
}

void RenderPassView::p_onTexture_(const QString &src, const QImage &img)
{
    p_->pixmaps.insert(src, QPixmap::fromImage(img.scaled(64,64)));

    for (size_t i=0; i<4; ++i)
    {
        Private::InputWidget& iw = p_->inputWidgets[i];
        if (iw.src == src)
            iw.imgLabel->setPixmap(p_->pixmaps[src]);
    }
}

void RenderPassView::Private::sourceEdited()
{
    int idx = tabBar->currentIndex();
    if (idx < 0 || (size_t)idx > shader.numRenderPasses())
        return;
    auto pass = shader.renderPass(idx);
    pass.setFragmentSource(textEdit->toPlainText());
    shader.setRenderPass(idx, pass);
    emit p->shaderChanged();
}


void RenderPassView::Private::inputsEdited()
{
    int idx = tabBar->currentIndex();
    if (idx < 0 || (size_t)idx > shader.numRenderPasses())
        return;
    auto pass = shader.renderPass(idx);
    for (size_t i=0; i<pass.numInputs(); ++i)
    {
        auto inp = pass.input(i);
        inp.vFlip = inputWidgets[i].vFlip->isChecked();
        inp.wrapMode = (ShadertoyInput::WrapMode)
                inputWidgets[i].wrap->currentData().toInt();
        inp.filterType = (ShadertoyInput::FilterType)
                inputWidgets[i].filter->currentData().toInt();
        pass.setInput(i, inp);
    }
    shader.setRenderPass(idx, pass);
    emit p->shaderChanged();
}
