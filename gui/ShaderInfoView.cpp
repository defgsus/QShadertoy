/***************************************************************************

Copyright (C) 2016  stefan.berke @ modular-audio-graphics.com

This source is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

****************************************************************************/

#include <QLayout>
#include <QLabel>
#include <QDebug>

#include "ShaderInfoView.h"
#include "core/ShadertoyShader.h"

struct ShaderInfoView::Private
{
    Private(ShaderInfoView* p)
        : p         (p)
    { }

    void createWidgets();
    void updateWidgets();

    QString toHtmlString(const QString& s) const;

    ShaderInfoView* p;
    ShadertoyShader shader;

    QLabel *labelName,
           *labelInfo,
           *labelWeb,
           *labelDesc;
};

ShaderInfoView::ShaderInfoView(QWidget *parent)
    : QWidget   (parent)
    , p_        (new Private(this))
{
    setMaximumWidth(300);
    p_->createWidgets();
}

ShaderInfoView::~ShaderInfoView()
{
    delete p_;
}

void ShaderInfoView::setShader(const ShadertoyShader& s)
{
    p_->shader = s;
    p_->updateWidgets();
}

void ShaderInfoView::Private::createWidgets()
{
    auto lv = new QVBoxLayout(p);
    lv->setMargin(0);

        labelName = new QLabel(p);
        QFont f = labelName->font();
        f.setBold(true);
        labelName->setFont(f);
        lv->addWidget(labelName);

        labelWeb = new QLabel(p);
        labelWeb->setOpenExternalLinks(true);
        labelWeb->setWordWrap(true);
        lv->addWidget(labelWeb);

        labelInfo = new QLabel(p);
        labelInfo->setWordWrap(true);
        lv->addWidget(labelInfo);

        labelDesc = new QLabel(p);
        labelDesc->setOpenExternalLinks(true);
        labelDesc->setWordWrap(true);
        lv->addWidget(labelDesc);


        lv->addStretch(1);
}

void ShaderInfoView::Private::updateWidgets()
{
    if (!shader.isValid())
    {
        labelName->clear();
        labelWeb->clear();
        labelInfo->clear();
        labelDesc->clear();
        return;
    }

    labelName->setText( shader.info().name );
    labelWeb->setText( QString("<html><a href=\"%1\">%1</a></html>")
                       .arg(shader.info().getUrl()) );
    labelInfo->setText( QString("user: %1\ndate: %2\nviews: %3\nlikes: %4")
                        .arg(shader.info().username)
                        .arg(shader.info().date.toString())
                        .arg(shader.info().views)
                        .arg(shader.info().likes));
    labelDesc->setText( toHtmlString(shader.info().description) );
}

QString ShaderInfoView::Private::toHtmlString(const QString& s1) const
{
    auto s = s1;
    s.replace("\n", "<br/>");

    // replace [url=src]desc[/url] with html tag
    int idx = s.indexOf("[url=", 0);
    while (idx != -1)
    {
        int idx2 = s.indexOf("[/url]", idx);
        int idx3 = s.indexOf("]", idx);
        if (idx2 == -1 || idx3 == -1)
            break;
        QString src = s.mid(idx+5, idx3-idx-5).trimmed();
        QString desc = s.mid(idx3+1, idx2-idx3-1);

        QString a = QString("<a href=\"%1\">%2</a>").arg(src).arg(desc);
        s.replace(idx, idx2-idx+6, a);

        idx = s.indexOf("[url=", idx + a.length());
    }

    // -- replace standalone weblinks --

    QRegExp urls("((http|https)?://|www.)[0-9A-Za-z%./=\\?]*");
    idx = urls.indexIn(s, 0);
    while (idx != -1)
    {
        qDebug() << "HTML[" << s.mid(idx, urls.matchedLength()) << "]";
        auto len = urls.matchedLength();
        // check if not already part of an <a> tag
        int idx2 = s.indexOf("</a", idx+len);
        if (idx2 != -1 && idx2 <= idx+len+3)
        {
            idx = urls.indexIn(s, idx+len);
            continue;
        }

        QString src = s.mid(idx, len);
        QString a = QString("<a href=\"%1\">%1</a>").arg(src);
        s.replace(idx, len, a);
        idx = urls.indexIn(s, idx + a.length());
    }

    return "<html>" + s + "</html>";
}
