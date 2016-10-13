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

#include "ShaderInfoView.h"
#include "core/ShadertoyShader.h"

struct ShaderInfoView::Private
{
    Private(ShaderInfoView* p)
        : p         (p)
    { }

    void createWidgets();
    void updateWidgets();

    ShaderInfoView* p;
    ShadertoyShader shader;

    QLabel *labelName,
           *labelDesc;
};

ShaderInfoView::ShaderInfoView(QWidget *parent)
    : QWidget   (parent)
    , p_        (new Private(this))
{
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
        lv->addWidget(labelName);

        labelDesc = new QLabel(p);
        lv->addWidget(labelDesc);
}

void ShaderInfoView::Private::updateWidgets()
{
    labelName->setText( shader.info().name );
    labelDesc->setText( shader.info().description );
}
