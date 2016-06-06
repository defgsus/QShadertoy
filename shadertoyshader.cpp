/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/
#include <QDebug>
#include <QJsonArray>

#include "shadertoyshader.h"


QString ShadertoyInput::nameForType(Type t)
{
    switch (t)
    {
        case T_TEXTURE: return "texture";
        case T_CUBEMAP: return "cubemap";
        case T_BUFFER: return "buffer";
        case T_KEYBOARD: return "keyboard";
        case T_MUSIC: return "music";
        case T_MUSICSTREAM: return "musicstream";
        default: return "*unknown*";
    }
}

QString ShadertoyInput::nameForType(FilterType t)
{
    switch (t)
    {
        case F_NEAREST: return "nearest";
        case F_LINEAR: return "linear";
        default: return "*unknown*";
    }
}

QString ShadertoyInput::nameForType(WrapMode t)
{
    switch (t)
    {
        case W_CLAMP: return "clamp";
        case W_REPEAT: return "repeat";
        default: return "*unknown*";
    }
}

QString ShadertoyRenderPass::nameForType(Type t)
{
    switch (t)
    {
        case T_IMAGE: return "Image";
        case T_SOUND: return "Sound";
        case T_BUFFER: return "Buffer";
        default: return "*unknown*";
    }
}




ShadertoyShader::ShadertoyShader()
{

}

bool ShadertoyShader::setJsonData(const QJsonObject& o)
{
    p_data_ = o;
    p_passes_.clear();
    p_error_.clear();

    // --- read info field ---

    auto inf = p_data_.value("info").toObject();
    if (inf.isEmpty())
    {
        p_error_ = ("missing info in json");
        return false;
    }
    p_info_.id = inf.value("id").toString();
    p_info_.views = inf.value("viewed").toInt();
    p_info_.likes = inf.value("likes").toInt();
    p_info_.published = inf.value("published").toInt();
    p_info_.flags = inf.value("flags").toInt();
    p_info_.hasliked = inf.value("hasliked").toInt();
    p_info_.name = inf.value("name").toString();
    p_info_.username = inf.value("username").toString();
    p_info_.description = inf.value("description").toString();
    auto secs = inf.value("date").toString().toLong();
    p_info_.date = QDateTime(QDate(1970, 1, 1)).addSecs(secs);
    p_info_.tags.clear();
    for (auto t : inf.value("tags").toArray())
        p_info_.tags << t.toString();

    p_info_.numChars = 0; // will be counted below
    p_info_.usesTextures = false;
    p_info_.usesMusic = false;

    // --- read render passes ---

    auto rp = p_data_.value("renderpass").toArray();
    if (rp.isEmpty())
    {
        p_error_ = ("missing renderpass in json");
        return false;
    }

    for (const QJsonValue& o : rp)
    {
        if (!o.isObject())
        {
            p_error_ = ("invalid renderpass in json");
            return false;
        }

        // -- prepare renderpass --

        ShadertoyRenderPass p;
        p.p_data_ = o.toObject();
        p.p_code_ = p.p_data_.value("code").toString();

        p_info_.numChars += p.p_code_.size();

        const QString type = p.p_data_.value("type").toString();
        p.p_type_ = ShadertoyRenderPass::T_IMAGE;
        if (type == "buffer")
            p.p_type_ = ShadertoyRenderPass::T_BUFFER;
        else if (type == "sound")
            p.p_type_ = ShadertoyRenderPass::T_SOUND;
        QString name = p.p_data_.value("name").toString();
        if (name.isEmpty())
            name = p.typeName();
        p.p_name_ = name;

        auto ins = p.p_data_.value("inputs").toArray();
        for (const QJsonValue& inv : ins)
        {
            QJsonObject in = inv.toObject();
            ShadertoyInput inp;
            inp.channel = in.value("channel").toInt();
            inp.id = in.value("id").toInt();
            inp.src = in.value("src").toString();
            inp.vFlip = in.value("vflip").toBool();

            const QString ctype = in.value("ctype").toString();
            if (ctype == "keyboard")
                inp.type = ShadertoyInput::T_KEYBOARD;
            else if (ctype == "musicstream")
            {
                p_info_.usesMusic = true;
                inp.type = ShadertoyInput::T_MUSICSTREAM;
            }
            else if (ctype == "music")
            {
                p_info_.usesMusic = true;
                inp.type = ShadertoyInput::T_MUSIC;
            }
            else if (ctype == "buffer")
                inp.type = ShadertoyInput::T_BUFFER;
            else if (ctype == "cubemap")
            {
                p_info_.usesTextures = true;
                inp.type = ShadertoyInput::T_CUBEMAP;
            }
            else
            {
                p_info_.usesTextures = true;
                inp.type = ShadertoyInput::T_TEXTURE;
            }

            const QString ftype = in.value("filter").toString();
            if (ftype == "linear")
                inp.filterType = ShadertoyInput::F_LINEAR;
            else
                inp.filterType = ShadertoyInput::F_NEAREST;

            const QString wtype = in.value("wrap").toString();
            if (ftype == "repeat")
                inp.wrapMode = ShadertoyInput::W_REPEAT;
            else
                inp.wrapMode = ShadertoyInput::W_CLAMP;

            p.p_inputs_.push_back(inp);
        }

        p_passes_.push_back(p);
    }

    // sort for correct execution order
    qSort(p_passes_.begin(), p_passes_.end(),
          [](const ShadertoyRenderPass& l, const ShadertoyRenderPass& r)
    {
        return l.type() < r.type();
    });

    return true;
}

bool ShadertoyShader::containsString(const QString& s) const
{
    if (p_info_.description.contains(s)
        || p_info_.name.contains(s)
        || p_info_.username.contains(s))
        return true;

    for (const QString& tag : p_info_.tags)
        if (tag.contains(s))
            return true;

    for (const ShadertoyRenderPass& p : p_passes_)
        if (p.fragmentSource().contains(s))
            return true;

    return false;
}
