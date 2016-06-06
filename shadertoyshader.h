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

struct ShadertoyInput
{
    enum Type
    {
        T_TEXTURE,
        T_CUBEMAP,
        T_BUFFER,
        T_KEYBOARD,
        T_MUSIC,
        T_MUSICSTREAM
    };
    static QString nameForType(Type);

    enum FilterType
    {
        F_NEAREST,
        F_LINEAR,
        F_MIPMAP
    };
    static QString nameForType(FilterType);

    enum WrapMode
    {
        W_CLAMP,
        W_REPEAT
    };
    static QString nameForType(WrapMode);

    int channel, id;
    Type type;
    FilterType filterType;
    WrapMode wrapMode;
    bool vFlip;
    QString src;

    QString typeName() const { return nameForType(type); }
    QString filterTypeName() const { return nameForType(filterType); }
    QString wrapModeName() const { return nameForType(wrapMode); }

    /** Description */
    QString toString() const;
};


class ShadertoyRenderPass
{
public:

    /** Order matters for correct sorting/execution */
    enum Type
    {
        T_BUFFER = 0,
        T_IMAGE = 1,
        T_SOUND = 2
    };
    static QString nameForType(Type);

    // ----- getter -----

    const QJsonObject& jsonData() const { return p_data_; }

    Type type() const { return p_type_; }
    QString typeName() const { return nameForType(p_type_); }
    QString name() const { return p_name_; }

    size_t numInputs() const { return p_inputs_.size(); }
    const ShadertoyInput& input(size_t idx) const { return p_inputs_[idx]; }

    const QString& fragmentSource() const { return p_code_; }


    // ----- setter -----

    void setFragmentSource(const QString&);
    void setInput(size_t idx, const ShadertoyInput& );

private:
    friend class ShadertoyShader;
    QJsonObject p_data_;
    QString p_code_, p_name_;
    Type p_type_;
    QVector<ShadertoyInput> p_inputs_;
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
         usesMusic;
};

class ShadertoyShader
{
public:
    ShadertoyShader();

    // ----- getter -----

    bool isValid() const { return !p_passes_.isEmpty() && p_error_.isEmpty(); }

    const QJsonObject& jsonData() { return p_data_; }

    const ShadertoyShaderInfo& info() const { return p_info_; }

    size_t numRenderPasses() const { return p_passes_.size(); }
    const ShadertoyRenderPass& renderPass(size_t idx) const { return p_passes_[idx]; }

    bool containsString(const QString&) const;

    // ----- setter -----

    bool setJsonData(const QJsonObject& );

    void setRenderPass(size_t index, const ShadertoyRenderPass& pass);

private:
    QJsonObject p_data_;
    QVector<ShadertoyRenderPass> p_passes_;
    QString p_error_;
    ShadertoyShaderInfo p_info_;
};

#endif // SHADERTOYSHADER_H
