/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/30/2016</p>
*/

#ifndef SHADERTOYSHADER_H
#define SHADERTOYSHADER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QVector>

/** Json-wrapper for a shadertoy input */
class ShadertoyInput
{
public:
    enum Type
    {
        T_NONE,
        T_TEXTURE,
        T_CUBEMAP,
        T_VIDEO,
        T_CAMERA,
        T_BUFFER,
        T_KEYBOARD,
        T_MICROPHONE,
        T_MUSIC,
        T_MUSICSTREAM
    };

    enum FilterType
    {
        F_NEAREST,
        F_LINEAR,
        F_MIPMAP
    };

    enum WrapMode
    {
        W_CLAMP,
        W_REPEAT
    };

    const QJsonObject& jsonData() const { return p_json; }

    bool isValid() const { return type() != T_NONE; }

    int channel() const;
    int id() const;
    Type type() const;
    FilterType filterType() const;
    WrapMode wrapMode() const;
    bool vFlip() const;
    QString source() const;
    QString typeName() const;
    QString filterTypeName() const;
    QString wrapModeName() const;

    bool hasFilterSetting() const;
    bool hasWrapSetting() const;
    bool hasVFlipSetting() const;

    /** Description */
    QString toString() const;

    // --- setter ---

    void setJsonData(const QJsonObject& o) { p_json = o; }

    void setChannel(int);
    void setId(int);
    void setType(Type);
    void setFilterType(FilterType);
    void setWrapMode(WrapMode);
    void setVFlip(bool);
    void setSource(const QString&);

private:
    QJsonObject p_json;
};



/** Json-wrapper for a single render pass */
class ShadertoyRenderPass
{
public:

    /** Order matters for correct execution order */
    enum Type
    {
        T_NONE = 0,
        T_BUFFER = 1,
        T_IMAGE = 2,
        T_SOUND = 3
    };

    ShadertoyRenderPass();

    // ----- getter -----

    const QJsonObject& jsonData() const { return p_json; }

    bool isValid() const { return type() != T_NONE; }

    Type type() const;
    QString typeName() const;
    QString name() const;
    int outputId() const;
    const QString& fragmentSource() const;

    size_t numInputs() const { return p_inputs.size(); }
    const ShadertoyInput& input(size_t idx) const { return p_inputs[idx]; }


    // ----- setter -----

    void setJsonData(const QJsonObject& o);

    void setFragmentSource(const QString&);
    void setInput(size_t idx, const ShadertoyInput& );

private:
    friend class ShadertoyShader;
    QJsonObject p_json;
    QVector<ShadertoyInput> p_inputs;
    QString p_fragSrc;
};



/** Info about a shader */
struct ShadertoyShaderInfo
{
    int views, likes, published, hasliked, flags;
    QString id, name, username, description;
    QDateTime date;
    QStringList tags;
    /** Number of characters of all passes */
    size_t numChars;
    /** Uses textures (not buffers) */
    bool usesTextures,
         usesBuffers,
         usesMusic,
         usesVideo,
         usesCamera,
         usesMicrophone,
         usesKeyboard;
};


/** Json-wrapper for a shadertoy program */
class ShadertoyShader
{
public:
    ShadertoyShader(const QString& id = "");

    // ----- getter -----

    bool isValid() const { return !p_passes_.isEmpty() && p_error_.isEmpty(); }

    const QJsonObject& jsonData() { return p_json; }

    const ShadertoyShaderInfo& info() const { return p_info_; }

    size_t numRenderPasses() const { return p_passes_.size(); }
    const ShadertoyRenderPass& renderPass(size_t idx) const
        { return p_passes_[idx]; }

    bool containsString(const QString&) const;

    // ----- setter -----

    bool setJsonData(const QJsonObject& );

    void setRenderPass(size_t index, const ShadertoyRenderPass& pass);

private:
    QJsonObject p_json;
    QVector<ShadertoyRenderPass> p_passes_;
    QString p_error_;
    ShadertoyShaderInfo p_info_;
};

#endif // SHADERTOYSHADER_H
