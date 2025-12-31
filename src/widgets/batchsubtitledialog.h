#ifndef BATCHSUBTITLEDIALOG_H
#define BATCHSUBTITLEDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui {
class BatchSubtitleDialog;
}

class BatchSubtitleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchSubtitleDialog(QWidget *parent = nullptr);
    ~BatchSubtitleDialog();

    // 获取批量编辑设置
    struct BatchSettings {
        bool applyToAll = false;
        int selectedTrack = -1;
        QString fontFamily;
        int fontSize = 24;
        QColor fontColor;
        QColor backgroundColor;
        double opacity = 1.0;
        QString alignment;
        bool addOutline = false;
        QColor outlineColor;
        int outlineWidth = 2;
        bool addShadow = false;
        QColor shadowColor;
        QPoint shadowOffset;
        int shadowBlur = 5;
    };

    BatchSettings getSettings() const;

private slots:
    void onSelectAllTracks();
    void onSelectTrack();
    void onColorChanged();
    void onPreview();
    void onApply();

private:
    Ui::BatchSubtitleDialog *ui;
    BatchSettings m_settings;
};

#endif // BATCHSUBTITLEDIALOG_H
