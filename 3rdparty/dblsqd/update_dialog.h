#ifndef DBLSQD_UPDATE_DIALOG_H
#define DBLSQD_UPDATE_DIALOG_H

#include "feed.h"
#include "ui_update_dialog.h"
#include <QSettings>
#include <QTemporaryFile>
#include <QFile>
#include <QErrorMessage>
#include <QDesktopServices>

namespace dblsqd {

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    enum Type {OnUpdateAvailable, OnLastWindowClosed, Manual};
    explicit UpdateDialog(Feed* feed, Type = Type::OnUpdateAvailable, QWidget* parent = 0, QSettings* settings = new QSettings());
    ~UpdateDialog();

    void setIcon(QString fileName);
    void setIcon(QPixmap pixmap);

signals:
    void ready();
    void aboutToShow();

public slots:
    void accept();
    void skip();
    void reject();
    void showIfUpdatesAvailable();

private:
    Ui::UpdateDialog* ui;
    Feed* feed;
    Type type;

    QSettings* settings;
    QString settingsGroup;
    void setSettingsValue(QString key, QVariant value);
    QVariant settingsValue(QString key, QVariant defaultValue = QVariant());
    void removeSetting(QString key);
    void replaceAppVars(QString& string);

    void disableButtons(bool disable = true);
    void toggleNoUpdates(bool noUpdates = true);

    void startDownload();
    void startUpdate();

    bool accepted;
    bool isDownloadFinished;
    QString updateFilePath;
    Release latestRelease;

private slots:
    void toggleAutoDownloads(bool);
    void handleFeedReady();
    void handleDownloadFinished();
    void handleDownloadError(QString);
    void updateProgressBar(qint64, qint64);
};

} //namespace dblsqd

#endif // DBLSQD_UPDATE_DIALOG_H
