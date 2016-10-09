/** @file

    @brief

    <p>(c) 2016, stefan.berke@modular-audio-graphics.com</p>
    <p>All rights reserved</p>

    <p>created 5/31/2016</p>
*/

#ifndef RENDERPASSVIEW_H
#define RENDERPASSVIEW_H

#include <QWidget>

class ShadertoyShader;

class RenderPassView : public QWidget
{
    Q_OBJECT
public:
    explicit RenderPassView(QWidget *parent = 0);
    ~RenderPassView();

    const ShadertoyShader& shader() const;

signals:

    /** Something has been changed (source or input settings) */
    void shaderChanged();

public slots:

    void setShader(const ShadertoyShader&);

private slots:

    void p_onTexture_(const QString& src, const QImage& img);

private:
    struct Private;
    Private* p_;
};

#endif // RENDERPASSVIEW_H
