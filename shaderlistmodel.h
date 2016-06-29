/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef SHADERLISTMODEL_H
#define SHADERLISTMODEL_H

#include <QAbstractTableModel>

class ShadertoyShader;

class ShaderListModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum Columns
    {
        C_ID,
        C_NAME,
        C_USER,
        C_DATE,
        C_VIEWS,
        C_LIKES,
        C_PASSES,
        C_NUM_CHARS,
        C_USE_TEX,
        C_USE_MUSIC,
        C_FLAGS
    };


    explicit ShaderListModel(QObject *parent = 0);
    ~ShaderListModel();

    const QStringList& shaderIds() const;

    ShadertoyShader getShader(const QModelIndex&) const;
    const ShadertoyShader& getShader(int row) const;

public slots:

    void loadShaders();

    bool setShader(const QString& id, const ShadertoyShader& s);

public:

    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
private:
    struct Private;
    Private* p_;
};

#endif // SHADERLISTMODEL_H
