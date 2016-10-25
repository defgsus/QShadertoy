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
class ShadertoyApi;

/** Model to hold a list of Shaders as table.
    Uses ShadertoyApi internally.
    */
class ShaderListModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    /** The different columns in the table */
    enum ColumnId
    {
        C_ID,
        C_NAME,
        C_IMAGE,
        C_USER,
        C_DATE,
        C_VIEWS,
        C_LIKES,
        C_PASSES,
        C_NUM_CHARS,
        C_HAS_SOUND,
        C_USE_TEX,
        C_USE_MUSIC,
        C_USE_KEYBOARD,
        C_USE_MOUSE,
        C_FLAGS
    };

    explicit ShaderListModel(QObject *parent = 0);
    ~ShaderListModel();

    const QStringList& shaderIds() const;

    /** Shader for index */
    ShadertoyShader getShader(const QModelIndex&) const;
    /** Shader for row, no error checking */
    const ShadertoyShader& getShader(int row) const;

    ShadertoyApi* api();

    /** Display snapshots images in the table */
    void setEnableThumbnails(bool e);

    // --- QAbstractTableModel interface ---

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
