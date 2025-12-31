// SubtitleEditorWidget.h
#ifndef SUBTITLEEDITORWIDGET_H
#define SUBTITLEEDITORWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>

class SubtitleEditorWidget : public QWidget {
    Q_OBJECT

public:
    SubtitleEditorWidget(QWidget *parent = nullptr);
    ~SubtitleEditor();

    void loadSubtitles(const QStringList &subtitles);
    QStringList saveSubtitles() const;

private slots:
    void on_editButton_clicked();

private:
    void setupUi();
    void connectSignalsAndSlots();

private:
    QStringList subtitles;
    QTableWidget *tableWidget;
};

#endif // SUBTITLEEDITORWIDGET_H
