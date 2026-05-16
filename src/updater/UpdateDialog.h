/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DBLSQD_UPDATE_DIALOG_H
#define DBLSQD_UPDATE_DIALOG_H

#include "Release.h"

#include <QDialog>
#include <QVariant>

#include <memory>

class QAbstractButton;
class QPixmap;
class QSettings;

namespace Ui {
class UpdateDialog;
}

namespace dblsqd {

class Feed;

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    enum Type { OnUpdateAvailable, OnLastWindowClosed, Manual, ManualChangelog };
    explicit UpdateDialog(Feed* feed, Type type, QSettings* settings, QWidget* parent = nullptr);
    ~UpdateDialog();

    void setIcon(const QString& fileName);
    void setIcon(const QPixmap& pixmap);
    void addInstallButton(QAbstractButton* button);

    void setMinVersion(const QString& version);
    void setMaxVersion(const QString& version);
    void setPreviousVersion(const QString& version);

    static bool autoDownloadEnabled(QVariant defaultValue, QSettings* settings);
    static bool autoDownloadEnabled(QSettings* settings);
    static void enableAutoDownload(bool enabled, QSettings* settings);

    void setOpenExternalLinks(bool open);
    bool openExternalLinks() const;

signals:
    void ready();
    void installButtonClicked(QAbstractButton* button, const QString& filePath);
    void linkActivated(const QString& link);

public slots:
    void onButtonInstall();
    void onButtonCustomInstall();
    void skip();
    void showIfUpdatesAvailable();
    void showIfUpdatesAvailableOrQuit();

private:
    std::unique_ptr<Ui::UpdateDialog> mUi;
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
    void updateWindowTitle();

    void startDownload();
    void startUpdate();

    bool mAccepted{false};
    bool mIsDownloadFinished{false};
    bool mFeedLoadFailed{false};
    QString mUpdateFilePath;
    QList<Release> mReleases;
    QList<Release> mUpdates;
    Release mLatestRelease;
    QList<QAbstractButton*> mInstallButtons;
    QAbstractButton* mAcceptedInstallButton;
    bool mOpenExternalLinks{true};
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
