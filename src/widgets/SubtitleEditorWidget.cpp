// SubtitleEditorWidget.cpp
#include "SubtitleEditorWidget.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidgetItem>

SubtitleEditorWidget::SubtitleEditor(QWidget *parent) {
    setupUi();
    connectSignalsAndSlots();
}

void SubtitleEditorWidget::setupUi() {
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(2); // 假设每行字幕有两个字段：时间码和文本
    tableWidget->setHorizontalHeaderLabels(QStringList() << "时间码" << "文本");
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tableWidget);
    QPushButton *editButton = new QPushButton("编辑", this);
    connect(editButton, &QPushButton::clicked, this, &SubtitleEditorWidget::on_editButton_clicked);
    layout->addWidget(editButton);
    setLayout(layout);
}

void SubtitleEditorWidget::connectSignalsAndSlots() {
    connect(editButton, &QPushButton::clicked, this, &SubtitleEditorWidget::on_editButton_clicked);
}

void SubtitleEditorWidget::loadSubtitles(const QStringList &subtitles) {
    tableWidget->setRowCount(subtitles.size());
    for (int i = 0; i < subtitles.size(); ++i) {
        tableWidget->setItem(i, 0, new QTableWidgetItem(subtitles[i].split("|")[0]));
        tableWidget->setItem(i, 1, new QTableWidgetItem(subtitles[i].split("|")[1]));
    }
}

QStringList SubtitleEditorWidget::saveSubtitles() const {
    QStringList savedSubtitles;
    for (int i = 0; i < tableWidget->rowCount(); ++i) {
        QString timeCode = tableWidget->item(i, 0)->text();
        QString text = tableWidget->item(i, 1)->text();
        savedSubtitles.append(QString("%1|%2").arg(timeCode).arg(text));
    }
    return savedSubtitles;
}

void SubtitleEditorWidget::on_editButton_clicked() {
    // 这里可以添加编辑字幕的逻辑
    QStringList subtitles = saveSubtitles();
    QMessageBox::information(this, "字幕已保存", "字幕已成功保存");
}
