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

#include <QTextStream>

#include "ShadertoyShader.h"
#include "log.h"

QString ShadertoyInput::toString() const
{
    QString str;
    QTextStream s(&str);
    s << typeName() << ", src=" << source()
      << ", chan=" << channel() << ", id=" << id()
      << ", wrap=" << wrapModeName()
      << ", filter=" << filterTypeName()
      << ", vflip=" << (vFlip() ? "yes" : "no")
         ;
    return str;
}

bool ShadertoyInput::hasFilterSetting() const
{
    auto t = type();
    return t == T_TEXTURE
        || t == T_VIDEO
        || t == T_CUBEMAP
        || t == T_BUFFER
            ;
}

bool ShadertoyInput::hasWrapSetting() const
{
    auto t = type();
    return t == T_TEXTURE
        || t == T_VIDEO
            ;
}

bool ShadertoyInput::hasVFlipSetting() const
{
    auto t = type();
    return t == T_TEXTURE
        || t == T_VIDEO
        || t == T_CUBEMAP
            ;
}


int ShadertoyInput::channel() const
{
    return p_json.value("channel").toInt();
}

int ShadertoyInput::id() const
{
    return p_json.value("id").toInt();
}

QString ShadertoyInput::source() const
{
    return p_json.value("src").toString();
}

QString ShadertoyInput::typeName() const
{
    return p_json.value("ctype").toString();
}

ShadertoyInput::Type ShadertoyInput::type() const
{
    const QString ctype = typeName();
    if (ctype == "keyboard")
        return ShadertoyInput::T_KEYBOARD;
    else if (ctype == "musicstream")
        return ShadertoyInput::T_MUSICSTREAM;
    else if (ctype == "music")
        return ShadertoyInput::T_MUSIC;
    else if (ctype == "microphone")
        return ShadertoyInput::T_MICROPHONE;
    else if (ctype == "webcam")
        return ShadertoyInput::T_CAMERA;
    else if (ctype == "buffer")
        return ShadertoyInput::T_BUFFER;
    else if (ctype == "video")
        return ShadertoyInput::T_VIDEO;
    else if (ctype == "cubemap")
        return ShadertoyInput::T_CUBEMAP;
    else if (ctype == "texture")
        return ShadertoyInput::T_TEXTURE;
    else
        return ShadertoyInput::T_NONE;
}

QString ShadertoyInput::filterTypeName() const
{
    auto sampler = p_json.value("sampler").toObject();
    return sampler.value("filter").toString();
}

ShadertoyInput::FilterType ShadertoyInput::filterType() const
{
    const QString ftype = filterTypeName();

    if (ftype == "linear")
        return ShadertoyInput::F_LINEAR;
    else if (ftype == "mipmap")
        return ShadertoyInput::F_MIPMAP;
    else
        return ShadertoyInput::F_NEAREST;
}

QString ShadertoyInput::wrapModeName() const
{
    auto sampler = p_json.value("sampler").toObject();
    return sampler.value("wrap").toString();
}

ShadertoyInput::WrapMode ShadertoyInput::wrapMode() const
{
    if (wrapModeName() == "repeat")
        return ShadertoyInput::W_REPEAT;
    else
        return ShadertoyInput::W_CLAMP;
}

bool ShadertoyInput::vFlip() const
{
    auto sampler = p_json.value("sampler").toObject();
    return sampler.value("vflip").toString() == "true";
}


void ShadertoyInput::setChannel(int c)
{
    p_json.insert("channel", c);
}

void ShadertoyInput::setId(int c)
{
    p_json.insert("id", c);
}

void ShadertoyInput::setType(Type t)
{
    switch (t)
    {
        case ShadertoyInput::T_TEXTURE:
            p_json.insert("ctype", "texture"); break;
        case ShadertoyInput::T_KEYBOARD:
            p_json.insert("ctype", "keyboard"); break;
        case ShadertoyInput::T_MUSICSTREAM:
            p_json.insert("ctype", "musicstream"); break;
        case ShadertoyInput::T_MUSIC:
            p_json.insert("ctype", "music"); break;
        case ShadertoyInput::T_MICROPHONE:
            p_json.insert("ctype", "microphone"); break;
        case ShadertoyInput::T_CAMERA:
            p_json.insert("ctype", "webcam"); break;
        case ShadertoyInput::T_BUFFER:
            p_json.insert("ctype", "buffer"); break;
        case ShadertoyInput::T_VIDEO:
            p_json.insert("ctype", "video"); break;
        case ShadertoyInput::T_CUBEMAP:
            p_json.insert("ctype", "cubemap"); break;
        case ShadertoyInput::T_NONE:
            return;
    }
}

void ShadertoyInput::setFilterType(FilterType t)
{
    auto sampler = p_json.value("sampler").toObject();

    switch (t)
    {
        case ShadertoyInput::F_NEAREST:
            sampler.insert("filter", "nearest"); break;
        case ShadertoyInput::F_LINEAR:
            sampler.insert("filter", "linear"); break;
        case ShadertoyInput::F_MIPMAP:
            sampler.insert("filter", "mipmap"); break;
        default:
            return;
    }
    p_json.insert("sampler", sampler);
}

void ShadertoyInput::setWrapMode(WrapMode m)
{
    auto sampler = p_json.value("sampler").toObject();

    switch (m)
    {
        case ShadertoyInput::W_CLAMP:
            sampler.insert("wrap", "clamp"); break;
        case ShadertoyInput::W_REPEAT:
            sampler.insert("wrap", "repeat"); break;
        default:
            return;
    }
    p_json.insert("sampler", sampler);
}

void ShadertoyInput::setVFlip(bool e)
{
    auto sampler = p_json.value("sampler").toObject();
    sampler.insert("vflip", e ? "true" : "false");
    p_json.insert("sampler", sampler);
}

void ShadertoyInput::setSource(const QString& s)
{
    p_json.insert("src", s);
}


