/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#include <QJsonArray>
#include <QTextStream>

#include "shadertoyshader.h"
#include "log.h"

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
        case F_MIPMAP: return "mipmap";
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

QString ShadertoyInput::toString() const
{
    QString str;
    QTextStream s(&str);
    s << typeName() << ", src=" << src
      << ", chan=" << channel << ", id=" << id
      << ", wrap=" << wrapModeName()
      << ", filter=" << filterTypeName()
      << ", vflip=" << (vFlip ? "yes" : "no")
         ;
    return str;
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

void ShadertoyRenderPass::setFragmentSource(const QString& s)
{
    p_code_ = s;
    p_data_.insert("code", QJsonValue(s));
}

void ShadertoyRenderPass::setInput(size_t idx, const ShadertoyInput& inp)
{
    if ((int)idx < p_inputs_.size())
        p_inputs_[idx] = inp;

    auto ins = p_data_.value("inputs").toArray();
    // Todo: append or insert
    if ((int)idx > ins.count())
        return;

    auto in = ins[idx].toObject();
    //in.insert("channel", inp.channel);
    //in.insert("id", inp.id);
    //in.insert("src", inp.src);
    in.insert("vflip", (int)inp.vFlip);
    switch (inp.type)
    {
        case ShadertoyInput::T_KEYBOARD:
            in.insert("ctype", "keyboard"); break;
        case ShadertoyInput::T_MUSICSTREAM:
            in.insert("ctype", "musicstream"); break;
        case ShadertoyInput::T_MUSIC:
            in.insert("ctype", "music"); break;
        case ShadertoyInput::T_BUFFER:
            in.insert("ctype", "buffer"); break;
        case ShadertoyInput::T_CUBEMAP:
            in.insert("ctype", "cubemap"); break;
        default: in.insert("ctype", "texture"); break;
    }
    if (inp.filterType == ShadertoyInput::F_LINEAR)
        in.insert("filter", "linear");
    else if (inp.filterType == ShadertoyInput::F_MIPMAP)
        in.insert("filter", "mipmap");
    else
        in.insert("filter", "nearest");
    if (inp.wrapMode == ShadertoyInput::W_REPEAT)
        in.insert("wrap", "repeat");
    else
        in.insert("wrap", "clamp");
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

            auto sampler = in.value("sampler").toObject();

            inp.vFlip = sampler.value("vflip").toBool();

            const QString ftype = sampler.value("filter").toString();

            if (ftype == "linear")
                inp.filterType = ShadertoyInput::F_LINEAR;
            else if (ftype == "mipmap")
                inp.filterType = ShadertoyInput::F_MIPMAP;
            else
                inp.filterType = ShadertoyInput::F_NEAREST;

            const QString wtype = sampler.value("wrap").toString();
            if (ftype == "clamp")
                inp.wrapMode = ShadertoyInput::W_CLAMP;
            else
                inp.wrapMode = ShadertoyInput::W_REPEAT;

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
    if (p_info_.description.contains(s, Qt::CaseInsensitive)
        || p_info_.name.contains(s)
        || p_info_.username.contains(s, Qt::CaseInsensitive))
        return true;

    for (const QString& tag : p_info_.tags)
        if (tag.contains(s, Qt::CaseInsensitive))
            return true;

    for (const ShadertoyRenderPass& p : p_passes_)
        if (p.fragmentSource().contains(s, Qt::CaseInsensitive))
            return true;

    return false;
}

void ShadertoyShader::setRenderPass(size_t index, const ShadertoyRenderPass &pass)
{
    // XXX Todo: append or insert
    if ((int)index >= p_passes_.size())
        return;

    p_passes_[index] = pass;

    auto rp = p_data_.value("renderpass").toArray();
    if ((int)index < rp.count())
        rp[index] = pass.jsonData();
}
