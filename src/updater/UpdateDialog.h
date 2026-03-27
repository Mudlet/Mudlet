#ifndef DBLSQD_UPDATE_DIALOG_H
#define DBLSQD_UPDATE_DIALOG_H

#include "Feed.h"

#include <QDialog>
#include <QVariant>

class QAbstractButton;
class QPixmap;
class QSettings;

namespace Ui {
class UpdateDialog;
}

namespace dblsqd {

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    enum Type { OnUpdateAvailable, OnLastWindowClosed, Manual, ManualChangelog };
    explicit UpdateDialog(Feed* feed, Type type, QSettings* settings, QWidget* parent = nullptr);
    ~UpdateDialog();

    void setIcon(QString fileName);
    void setIcon(QPixmap pixmap);
    void addInstallButton(QAbstractButton* button);

    void setMinVersion(QString version);
    void setMaxVersion(QString version);
    void setPreviousVersion(QString version);

    static bool autoDownloadEnabled(QVariant defaultValue, QSettings* settings);
    static bool autoDownloadEnabled(QSettings* settings);
    static void enableAutoDownload(bool enabled, QSettings* settings);

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
    Ui::UpdateDialog* mUi;
    Feed* mFeed;
    Type mType;

    QSettings* mSettings;
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

    bool mAccepted;
    bool mIsDownloadFinished;
    QString mUpdateFilePath;
    QList<Release> mReleases;
    QList<Release> mUpdates;
    Release mLatestRelease;
    QList<QAbstractButton*> mInstallButtons;
    QAbstractButton* mAcceptedInstallButton;
    bool mOpenExternalLinks;
    QString mMinVersion;
    QString mMaxVersion;
    QString mPreviousVersion;

    static void setSettingsValue(const QString& key, const QVariant& value, QSettings* settings);
    static QVariant settingsValue(const QString& key, const QVariant& defaultValue, QSettings* settings);
    static void removeSetting(const QString& key, QSettings* settings);
    static void setDefaultSettingsValue(const QString& key, const QVariant& value, QSettings* settings);

private slots:
    void handleFeedReady();
    void handleLoadError(const QString& message);
    void handleDownloadFinished();
    void handleDownloadError(const QString& message);
    void updateProgressBar(qint64, qint64);
    void autoDownloadCheckboxToggled(bool enabled = true);
    void onLinkActivated(const QUrl& link);
};

} // namespace dblsqd

#endif // DBLSQD_UPDATE_DIALOG_H
