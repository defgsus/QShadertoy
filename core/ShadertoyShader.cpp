/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#include <QJsonArray>
#include <QTextStream>

#include "ShadertoyShader.h"
#include "log.h"


QString ShadertoyShaderInfo::getUrl() const
{
    return QString("https://shadertoy.com/view/%1").arg(id);
}


ShadertoyShader::ShadertoyShader(const QString& id)
{
    p_info_.id = id;
}

bool ShadertoyShader::setJsonData(const QJsonObject& o)
{
    p_json = o;
    p_passes_.clear();
    p_error_.clear();

    // --- read info field ---

    auto inf = p_json.value("info").toObject();
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
    { auto secs = inf.value("date").toString().toLong();
    p_info_.date = QDateTime(QDate(1970, 1, 1)).addSecs(secs); }
    p_info_.tags.clear();
    for (auto t : inf.value("tags").toArray())
        p_info_.tags << t.toString();

    p_info_.numChars = 0; // will be counted below
    p_info_.usesTextures = false;
    p_info_.usesBuffers = false;
    p_info_.usesMusic = false;
    p_info_.usesVideo = false;
    p_info_.usesCamera = false;
    p_info_.usesMicrophone = false;
    p_info_.usesKeyboard = false;

    // --- read render passes ---

    auto rp = p_json.value("renderpass").toArray();
    if (rp.isEmpty())
    {
        p_error_ = ("missing renderpass in json");
        return false;
    }

    for (const QJsonValue& jpass : rp)
    {
        if (!jpass.isObject())
        {
            p_error_ = ("invalid renderpass in json");
            return false;
        }


        ShadertoyRenderPass pass;
        pass.setJsonData(jpass.toObject());
        if (!pass.isValid())
        {
            p_error_ = ("invalid renderpass");
            return false;
        }
        p_passes_.push_back(pass);

        // -- update info --

        p_info_.numChars += pass.fragmentSource().size();

        for (size_t i = 0; i < pass.numInputs(); ++i)
        {
            switch (pass.input(i).type())
            {
                case ShadertoyInput::T_KEYBOARD:
                    p_info_.usesKeyboard = true;
                break;
                case ShadertoyInput::T_MUSIC:
                case ShadertoyInput::T_MUSICSTREAM:
                    p_info_.usesMusic = true;
                break;
                case ShadertoyInput::T_MICROPHONE:
                    p_info_.usesMicrophone = true;
                break;
                case ShadertoyInput::T_CAMERA:
                    p_info_.usesCamera = true;
                break;
                case ShadertoyInput::T_BUFFER:
                    p_info_.usesBuffers = true;
                break;
                case ShadertoyInput::T_VIDEO:
                    p_info_.usesVideo = true;
                break;
                case ShadertoyInput::T_TEXTURE:
                case ShadertoyInput::T_CUBEMAP:
                    p_info_.usesTextures = true;
                break;
                case ShadertoyInput::T_NONE:
                break;
            }
        }
    }

    return true;
}

QVector<ShadertoyRenderPass> ShadertoyShader::sortedRenderPasses() const
{
    auto passes = p_passes_;
    // sort for correct execution order
    qSort(passes.begin(), passes.end(),
          [](const ShadertoyRenderPass& l, const ShadertoyRenderPass& r)
    {
        // sorts "Buf X" alphabetically
        if (l.type() == r.type())
            return l.name() < r.name();
        // otherwise by type
        return l.type() < r.type();
    });
    return passes;
}

bool ShadertoyShader::containsString(const QString& s) const
{
    if (p_info_.description.contains(s, Qt::CaseInsensitive)
        || p_info_.name.contains(s, Qt::CaseInsensitive)
        || p_info_.username.contains(s, Qt::CaseInsensitive)
        || p_info_.id.contains(s, Qt::CaseSensitive)
        ) return true;

    for (const QString& tag : p_info_.tags)
        if (tag.contains(s, Qt::CaseInsensitive))
            return true;

    for (const ShadertoyRenderPass& p : p_passes_)
        if (p.fragmentSource().contains(s, Qt::CaseInsensitive))
            return true;

    return false;
}

void ShadertoyShader::setRenderPass(
        size_t index, const ShadertoyRenderPass &pass)
{
    auto rp = p_json.value("renderpass").toArray();

    // XXX Todo: append or insert
    if ((int)index >= rp.size())
    {
        ST_WARN("ShadertoyShader::setRenderPass("<<index<<") out of range");
        return;
    }

    p_passes_.resize(std::max(p_passes_.size(), (int)index+1));
    p_passes_[index] = pass;

    rp[index] = pass.jsonData();
}
