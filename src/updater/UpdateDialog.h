#ifndef DBLSQD_UPDATE_DIALOG_H
#define DBLSQD_UPDATE_DIALOG_H

#include "Feed.h"
#include "ui_update_dialog.h"
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryFile>
#include <QUrl>

namespace dblsqd {

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    enum Type { OnUpdateAvailable, OnLastWindowClosed, Manual, ManualChangelog };
    explicit UpdateDialog(Feed* feed, Type type = OnUpdateAvailable, QWidget* parent = nullptr, QSettings* settings = nullptr);
    ~UpdateDialog();

    void setIcon(QString fileName);
    void setIcon(QPixmap pixmap);
    void addInstallButton(QAbstractButton* button);

    void setMinVersion(QString version);
    void setMaxVersion(QString version);
    void setPreviousVersion(QString version);

    static bool autoDownloadEnabled(QVariant defaultValue, QSettings* settings = nullptr);
    static bool autoDownloadEnabled(QSettings* settings = nullptr);
    static void enableAutoDownload(bool enabled, QSettings* settings = nullptr);

    void setOpenExternalLinks(bool open);
    bool openExternalLinks();

signals:
    void ready();
    void installButtonClicked(QAbstractButton* button, QString filePath);
    void linkActivated(QString link);

public slots:
    void onButtonInstall();
    void onButtonCustomInstall();
    void skip();
    void showIfUpdatesAvailable();
    void showIfUpdatesAvailableOrQuit();

private:
    Ui::UpdateDialog* ui;
    Feed* feed;
    int type;

    QSettings* settings;
    void replaceAppVars(QString& string);
    QString generateChangelogDocument();

    void disableButtons(bool disable = true);
    void resetUi();
    void setupLoadingUi();
    void setupUpdateUi();
    void setupChangelogUi();
    void setupNoUpdatesUi();
    void adjustDialogSize();

    void startDownload();
    virtual void startUpdate();

    bool accepted;
    bool isDownloadFinished;
    QString updateFilePath;
    QList<Release> releases;
    QList<Release> updates;
    Release latestRelease;
    QList<QAbstractButton*> installButtons;
    QAbstractButton* acceptedInstallButton;
    bool _openExternalLinks;
    QString _minVersion;
    QString _maxVersion;
    QString _previousVersion;

    static void setSettingsValue(QString key, QVariant value, QSettings* settings = nullptr);
    static QVariant settingsValue(QString key, QVariant defaultValue = QVariant(), QSettings* settings = nullptr);
    static void removeSetting(QString key, QSettings* settings = nullptr);
    static void setDefaultSettingsValue(QString key, QVariant value, QSettings* settings = nullptr);

private slots:
    void handleFeedReady();
    void handleLoadError(QString message);
    void handleDownloadFinished();
    void handleDownloadError(QString);
    void updateProgressBar(qint64, qint64);
    void autoDownloadCheckboxToggled(bool enabled = true);
    void onLinkActivated(QUrl link);
};

} // namespace dblsqd

#endif // DBLSQD_UPDATE_DIALOG_H
