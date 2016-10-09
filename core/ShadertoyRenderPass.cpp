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

#include <QJsonArray>

#include "ShadertoyShader.h"
#include "log.h"

ShadertoyRenderPass::ShadertoyRenderPass()
    : p_inputs  (4)
{

}

ShadertoyRenderPass::Type ShadertoyRenderPass::type() const
{
    const QString t = typeName();
    if (t == "buffer")
        return T_BUFFER;
    else if (t == "sound")
        return T_SOUND;
    else if (t == "image")
        return T_IMAGE;
    else
        return T_NONE;
}

QString ShadertoyRenderPass::typeName() const
{
    return p_json.value("type").toString();
}

QString ShadertoyRenderPass::name() const
{
    QString s = p_json.value("name").toString();
    return s.isEmpty() ? typeName() : s;
}

int ShadertoyRenderPass::outputId() const
{
    // XXX Takes the first output id (no multi-target yet)
    auto outputs = p_json.value("outputs").toArray();
    if (outputs.isEmpty())
        return -1;
    else
        return outputs[0].toObject().value("id").toInt();
}

const QString& ShadertoyRenderPass::fragmentSource() const
{
    return p_fragSrc;
}


void ShadertoyRenderPass::setJsonData(const QJsonObject& o)
{
    p_json = o;
    p_fragSrc = p_json.value("code").toString();

    p_inputs.clear();

    auto ins = p_json.value("inputs").toArray();
    for (const QJsonValue& inv : ins)
    {
        ShadertoyInput inp;
        inp.setJsonData( inv.toObject() );

        p_inputs.resize(std::max(p_inputs.size(), inp.channel()+1));
        p_inputs[inp.channel()] = inp;
    }
}

void ShadertoyRenderPass::setFragmentSource(const QString& s)
{
    p_json.insert("code", QJsonValue(s));
    p_fragSrc = s;
}

void ShadertoyRenderPass::setInput(size_t idx, const ShadertoyInput& inp)
{
    QJsonArray ins = p_json.value("inputs").toArray();

    // Todo: append or insert
    if ((int)idx >= ins.count())
    {
        ST_WARN("ShadertoyRenderPass::setInput(" << idx << ") out of range");
        return;
    }

    p_inputs.resize(std::max((int)idx+1, p_inputs.size()));
    p_inputs[idx] = inp;

    for (int i=0; i<ins.size(); ++i)
        if (ins[idx].toObject().value("channel").toInt() == (int)idx)
            ins[i] = inp.jsonData();
}



