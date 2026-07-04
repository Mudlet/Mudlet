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

#include "UpdateDialog.h"
#include "Feed.h"
#include "ui_update_dialog.h"

#include "../utils.h"
#include "../../3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QRegularExpression>
#include <QSettings>
#include <QTextBrowser>
#include <QToolButton>

namespace dblsqd {

/*!
 * \class UpdateDialog
 * \brief A dialog class for displaying and downloading update information.
 *
 * UpdateDialog displays available updates from the GitHub Releases feed
 * and provides download/install functionality.
 *
 * The update dialog can also display an application icon which can be set with
 * setIcon().
 */

/*!
 * \enum UpdateDialog::Type
 * \brief This flag determines the if and when the UpdateDialog is displayed
 * automatically.
 *
 * *OnUpdateAvailable*: Automatically display the dialog as soon as the Feed
 * has been downloaded and parsed and if there is a newer version than the
 * current version returned by QCoreApplication::applicationVersion().
 *
 * *OnLastWindowClosed*: If there is a newer version available than the current
 * version returned by QCoreApplication::applicationVersion(), the update
 * dialog is displayed when QGuiApplication emits the lastWindowClosed() event.
 * Note that when this flag is used,
 * QGuiApplication::setQuitOnLastWindowClosed(false) will be called.
 *
 * *Manual*: The dialog is only displayed when explicitly requested via show()
 * or exec().
 * Note that update information might not be available instantly after
 * constructing an UpdateDialog.
 *
 * *ManualChangelog*: The dialog is only displayed when explicitly requested via
 * show() or exec().
 * Instead of the full update interface, only the changelog will be shown.
 */

/*!
 * \brief Constructs a new UpdateDialog.
 *
 * A Feed object needs to be constructed first and passed to this constructor.
 * Feed::load() does not need to be called before constructing this dialog --
 * the constructor calls it automatically if needed.
 *
 * The given UpdateDialog::Type flag determines when/if the dialog is shown
 * automatically.
 *
 * A QSettings object must be provided for persisting user preferences such as
 * skipped releases and auto-download settings.
 *
 */
UpdateDialog::UpdateDialog(Feed* feed, Type type, QSettings* settings, QWidget* parent)
: QDialog(parent)
, mUi(std::make_unique<Ui::UpdateDialog>())
, mFeed(feed)
, mType(type)
, mSettings(settings)
, mAcceptedInstallButton(nullptr)
{
    Q_ASSERT_X(feed, "UpdateDialog", "Feed object is required");
    Q_ASSERT_X(settings, "UpdateDialog", "QSettings object is required");
    mUi->setupUi(this);

    mUi->buttonCancel->addAction(mUi->actionCancel);
    mUi->buttonCancel->addAction(mUi->actionSkip);
    mUi->buttonCancel->setDefaultAction(mUi->actionCancel);

    connect(mUi->labelChangelog, &QTextBrowser::anchorClicked, this, &UpdateDialog::onLinkActivated);

    switch (mType) {
    case OnUpdateAvailable: {
        connect(this, &UpdateDialog::ready, this, &UpdateDialog::showIfUpdatesAvailable);
        break;
    }
    case OnLastWindowClosed: {
        auto* app = qobject_cast<QGuiApplication*>(QApplication::instance());
        app->setQuitOnLastWindowClosed(false);
        connect(app, &QGuiApplication::lastWindowClosed, this, &UpdateDialog::showIfUpdatesAvailableOrQuit);
        break;
    }
    case Manual:
    case ManualChangelog:
        break;
    }

    if (mFeed->isReady()) {
        handleFeedReady();
    } else {
        setupLoadingUi();
        mFeed->load();
        KDToolBox::connectSingleShot(mFeed, &Feed::ready, this, &UpdateDialog::handleFeedReady);
        KDToolBox::connectSingleShot(mFeed, &Feed::loadError, this, &UpdateDialog::handleLoadError);
    }
}

UpdateDialog::~UpdateDialog() = default;

/*!
 * \brief Sets the icon displayed in the update window.
 */
void UpdateDialog::setIcon(const QPixmap& pixmap)
{
    mUi->labelIcon->setPixmap(pixmap);
    mUi->labelIcon->setHidden(false);
}

void UpdateDialog::setIcon(const QString& fileName)
{
    mUi->labelIcon->setPixmap(QPixmap(fileName));
    mUi->labelIcon->setHidden(false);
}

/*!
 * \brief Sets the minimum version to be displayed in the changelog.
 * Defaults to QApplication::applicationVersion() if not set.
 * \param version
 */
void UpdateDialog::setMinVersion(const QString& version)
{
    mMinVersion = version;
    setupChangelogUi();
}

/*!
 * \brief Sets the maximum version to be displayed in the changelog.
 */
void UpdateDialog::setMaxVersion(const QString& version)
{
    mMaxVersion = version;
    setupChangelogUi();
}

/*!
 * \brief Convenience method for setting minimum and maximum version to be
 * displayed in the changelog. maximumVersion is set to
 * QApplication::applicationVersion().
 */
void UpdateDialog::setPreviousVersion(const QString& previousVersion)
{
    mPreviousVersion = previousVersion;
    mMinVersion = previousVersion;
    mMaxVersion = QApplication::applicationVersion();
    setupChangelogUi();
}

/*!
 * \brief Adds a custom button for handling update installation.
 * \param button
 *
 * When the custom button is clicked after an update has been downloaded or when
 * downloading an update that was started by clicking the button has finished,
 * installButtonClicked(QAbstractButton* button, QString filePath) is emitted.
 */
void UpdateDialog::addInstallButton(QAbstractButton* button)
{
    mInstallButtons.append(button);
    mUi->buttonContainer->layout()->addWidget(button);
    if (isVisible() && mUi->buttonCancel->isVisible()) {
        setupUpdateUi();
    }
}

/*!
 * \brief Returns whether links in the changelog are opened externally.
 *
 * Determines if links in the changelog should be opened automatically by
 * QDesktopServices::openUrl() when a user clicks on them.
 * If set to false, the linkActivated() signal is emitted instead.
 *
 * The default value is true.
 */
bool UpdateDialog::openExternalLinks() const
{
    return mOpenExternalLinks;
}

/*!
 * \brief Sets whether links in the changelog are opened externally.
 */
void UpdateDialog::setOpenExternalLinks(bool open)
{
    mOpenExternalLinks = open;
}

/*!
 * \brief Default handler for the install button.
 *
 * Closes the dialog if no other action (such as
 * downloading or installing a Release) is required first.
 */
void UpdateDialog::onButtonInstall()
{
    mAccepted = true;
    mAcceptedInstallButton = nullptr;
    if (mIsDownloadFinished) {
        startUpdate();
    } else if (!mLatestRelease.getVersion().isEmpty()) {
        startDownload();
    } else {
        done(QDialog::Accepted);
    }
}

void UpdateDialog::onButtonCustomInstall()
{
    mAccepted = true;
    if (mIsDownloadFinished) {
        emit installButtonClicked(qobject_cast<QAbstractButton*>(sender()), mUpdateFilePath);
    } else if (!mLatestRelease.getVersion().isEmpty()) {
        mAcceptedInstallButton = qobject_cast<QAbstractButton*>(sender());
        startDownload();
    } else {
        done(QDialog::Accepted);
    }
}

/*!
 * \brief Skips the latest retrieved Release.
 *
 * If a release has been skipped, UpdateDialog will not be displayed
 * automatically when using Type::OnUpdateAvailable or
 * Type::OnLastWindowClosed.
 */
void UpdateDialog::skip()
{
    if (!mUpdateFilePath.isEmpty()) {
        if (!QFile::remove(mUpdateFilePath)) {
            qWarning() << "Failed to remove update file:" << mUpdateFilePath;
        }
    }
    setSettingsValue(qsl("skipRelease"), mLatestRelease.getVersion(), mSettings);
    done(QDialog::Rejected);
}

/*!
 * \brief Shows the dialog if there are available updates.
 */
void UpdateDialog::showIfUpdatesAvailable()
{
    QString latestVersion = mLatestRelease.getVersion();
    bool skipRelease = (settingsValue(qsl("skipRelease"), "", mSettings).toString() == latestVersion);
    if (!latestVersion.isEmpty() && !skipRelease) {
        show();
    }
}

/*!
 * \brief Shows the dialog if there are updates available or quits the
 * application.
 */
void UpdateDialog::showIfUpdatesAvailableOrQuit()
{
    if (mType == OnLastWindowClosed) {
        auto* app = qobject_cast<QGuiApplication*>(QApplication::instance());
        app->setQuitOnLastWindowClosed(true);
        disconnect(app, &QGuiApplication::lastWindowClosed, this, &UpdateDialog::showIfUpdatesAvailableOrQuit);
    }
    QString latestVersion = mLatestRelease.getVersion();
    bool skipRelease = (settingsValue(qsl("skipRelease"), "", mSettings).toString() == latestVersion);
    if (!latestVersion.isEmpty() && !skipRelease) {
        show();
    } else {
        QCoreApplication::quit();
    }
}

// "DBLSQD/" prefix retained for backward compatibility with user settings from the previous update system
QVariant UpdateDialog::settingsValue(const QString& key, const QVariant& defaultValue, QSettings* settings)
{
    return settings->value(qsl("DBLSQD/") + key, defaultValue);
}

void UpdateDialog::setSettingsValue(const QString& key, const QVariant& value, QSettings* settings)
{
    settings->setValue(qsl("DBLSQD/") + key, value);
}

void UpdateDialog::removeSetting(const QString& key, QSettings* settings)
{
    settings->remove(qsl("DBLSQD/") + key);
}

void UpdateDialog::setDefaultSettingsValue(const QString& key, const QVariant& value, QSettings* settings)
{
    if (settings->contains(qsl("DBLSQD/") + key))
        return;
    setSettingsValue(key, value, settings);
}

/*!
 * \brief Enables or disables automatic downloads.
 */
void UpdateDialog::enableAutoDownload(bool enabled, QSettings* settings)
{
    setSettingsValue(qsl("autoDownload"), enabled, settings);
}

/*!
 * \brief Returns true if automatic downloads are enabled.
 *
 * If defaultValue is provided, it is stored if no other value has previously
 * been set.
 */
bool UpdateDialog::autoDownloadEnabled(QVariant defaultValue, QSettings* settings)
{
    if (defaultValue.isValid()) {
        setDefaultSettingsValue(qsl("autoDownload"), defaultValue, settings);
    } else {
        defaultValue = false;
    }
    return settingsValue(qsl("autoDownload"), defaultValue, settings).toBool();
}

/*!
 * \overload
 */
bool UpdateDialog::autoDownloadEnabled(QSettings* settings)
{
    return settingsValue(qsl("autoDownload"), false, settings).toBool();
}

void UpdateDialog::adjustDialogSize()
{
    adjustSize();

/*HACK: Qt seems to incorrectly calculate window geometry on Windows.
        This code avoids warning messages logged by the application
        in that case.*/
#if defined(Q_OS_WINDOWS)
    QSize dialogSize = size();
    resize(dialogSize.width(), dialogSize.height() + 3);
#endif
}

void UpdateDialog::updateWindowTitle()
{
    QString title = windowTitle();
    replaceAppVars(title);
    setWindowTitle(title);
}

void UpdateDialog::resetUi()
{
    QList<QWidget*> hiddenWidgets;
    for (auto* button : mInstallButtons) {
        hiddenWidgets << button;
    }
    hiddenWidgets << mUi->headerContainer << mUi->labelIcon << mUi->headerContainerLoading << mUi->headerContainerNoUpdates << mUi->headerContainerChangelog << mUi->labelChangelog << mUi->progressBar
                  << mUi->checkAutoDownload << mUi->buttonCancel << mUi->buttonCancelLoading << mUi->buttonConfirm << mUi->buttonInstall;
    for (auto* widget : hiddenWidgets) {
        widget->hide();
        widget->disconnect();
    }
    // Re-establish the changelog link handler broken by disconnect() above
    connect(mUi->labelChangelog, &QTextBrowser::anchorClicked, this, &UpdateDialog::onLinkActivated);
    mUi->progressBar->reset();
    adjustDialogSize();
}

void UpdateDialog::setupLoadingUi()
{
    resetUi();
    mUi->headerContainerLoading->show();
    mUi->progressBar->show();
    mUi->progressBar->setMaximum(0);
    mUi->progressBar->setMinimum(0);
    mUi->buttonCancelLoading->show();
    mUi->buttonCancelLoading->setFocus();
    connect(mUi->buttonCancelLoading, &QPushButton::clicked, this, &UpdateDialog::reject);
    adjustDialogSize();
}

void UpdateDialog::setupUpdateUi()
{
    resetUi();

    QList<QWidget*> showWidgets;
    showWidgets << mUi->headerContainer << mUi->labelChangelog << mUi->checkAutoDownload << mUi->buttonCancel << mUi->buttonInstall;
    for (auto* widget : showWidgets) {
        widget->show();
    }

    for (auto* label : {mUi->labelHeadline, mUi->labelInfo}) {
        QString text = label->text();
        replaceAppVars(text);
        label->setText(text);
    }
    mUi->labelChangelog->setMarkdown(generateChangelogDocument());

    mUi->checkAutoDownload->setChecked(autoDownloadEnabled(mSettings));

    updateWindowTitle();

    // Show completed progress bar if release has been downloaded already
    if (mIsDownloadFinished) {
        mUi->progressBar->show();
        mUi->progressBar->setMaximum(1);
        mUi->progressBar->setValue(1);
    }

    connect(mFeed, &Feed::downloadFinished, this, &UpdateDialog::handleDownloadFinished, Qt::UniqueConnection);
    connect(mFeed, &Feed::downloadError, this, &UpdateDialog::handleDownloadError, Qt::UniqueConnection);
    connect(mFeed, &Feed::downloadProgress, this, &UpdateDialog::updateProgressBar, Qt::UniqueConnection);

    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    connect(mUi->actionCancel, &QAction::triggered, this, &UpdateDialog::reject);
    connect(mUi->actionSkip, &QAction::triggered, this, &UpdateDialog::skip);
    connect(mUi->checkAutoDownload, &QCheckBox::toggled, this, &UpdateDialog::autoDownloadCheckboxToggled);

    if (mInstallButtons.isEmpty()) {
        mUi->buttonInstall->setFocus();
        connect(mUi->buttonInstall, &QPushButton::clicked, this, &UpdateDialog::onButtonInstall);
    } else {
        mUi->buttonInstall->hide();
        for (auto* button : mInstallButtons) {
            button->show();
            connect(button, &QAbstractButton::clicked, this, &UpdateDialog::onButtonCustomInstall);
        }
        mInstallButtons.last()->setFocus();
    }

    adjustDialogSize();
}

void UpdateDialog::setupChangelogUi()
{
    resetUi();

    QList<QWidget*> showWidgets;
    showWidgets << mUi->headerContainerChangelog << mUi->buttonConfirm << mUi->labelChangelog;
    for (auto* widget : showWidgets) {
        widget->show();
    }
    for (auto* label : {mUi->labelHeadlineChangelog, mUi->labelInfoChangelog}) {
        QString text = label->text();
        replaceAppVars(text);
        label->setText(text);
    }

    updateWindowTitle();

    mUi->labelChangelog->setMarkdown(generateChangelogDocument());
    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    mUi->buttonConfirm->setFocus();
    adjustDialogSize();
}

void UpdateDialog::setupNoUpdatesUi()
{
    resetUi();
    QList<QWidget*> showWidgets;
    showWidgets << mUi->headerContainerNoUpdates << mUi->buttonConfirm;
    for (auto* widget : showWidgets) {
        widget->show();
    }
    mUi->buttonConfirm->setFocus();

    QString text = mUi->labelHeadlineNoUpdates->text();
    replaceAppVars(text);
    mUi->labelHeadlineNoUpdates->setText(text);

    updateWindowTitle();

    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    adjustDialogSize();
}

void UpdateDialog::disableButtons(bool disable)
{
    for (auto* button : mInstallButtons) {
        button->setDisabled(disable);
    }
    QList<QWidget*> buttons;
    buttons << mUi->buttonCancel << mUi->buttonCancelLoading << mUi->buttonConfirm << mUi->buttonInstall << mUi->checkAutoDownload;
    for (auto* button : buttons) {
        button->setDisabled(disable);
    }
}

void UpdateDialog::replaceAppVars(QString& string)
{
    string.replace("%APPNAME%", QCoreApplication::applicationName());
    string.replace("%CURRENT_VERSION%", QCoreApplication::applicationVersion());
    string.replace("%UPDATE_VERSION%", mLatestRelease.getVersion());
}

QString UpdateDialog::generateChangelogDocument()
{
    QString changelog;
    QList<Release> changelogReleases;
    if (mMinVersion.isEmpty() && mMaxVersion.isEmpty()) {
        changelogReleases = mUpdates;
        changelog = generateCompareLink();
    } else {
        Release minRelease(mMinVersion.isEmpty() ? QApplication::applicationVersion() : mMinVersion);
        Release maxRelease(mMaxVersion);
        for (const auto& release : mReleases) {
            if (minRelease < release && (mMaxVersion.isEmpty() || release <= maxRelease)) {
                changelogReleases << release;
            }
        }
    }
    static const QRegularExpression summaryTag(qsl("<details>\\s*<summary>(.*?)</summary>"));
    for (const auto& release : changelogReleases) {
        if (!changelog.isEmpty()) {
            changelog.append(qsl("---\n\n"));
        }
        QString body = release.getChangelog();
        // Convert <details>/<summary> to plain markdown - Qt's markdown
        // renderer can't handle these HTML5 tags, causing garbled output
        body.replace(summaryTag, qsl("#### \\1"));
        body.remove(qsl("</details>"));

        changelog.append(body + qsl("\n\n"));
    }
    return changelog;
}

QString UpdateDialog::generateCompareLink() const
{
    const QString installedVersion = QApplication::applicationVersion();
    const QString updateVersion = mLatestRelease.getVersion();
    if (installedVersion.isEmpty() || updateVersion.isEmpty()) {
        return QString();
    }
    const QString base = Release::gitHubRef(installedVersion);
    const QString head = Release::gitHubRef(updateVersion);
    if (base == head || mFeed->getOwner().isEmpty() || mFeed->getRepo().isEmpty()) {
        return QString();
    }
    const QString url = qsl("https://github.com/%1/%2/compare/%3...%4").arg(mFeed->getOwner(), mFeed->getRepo(), base, head);
    //: Shown above the update changelog; the text in [] is a clickable link, %1 is the GitHub comparison URL
    return tr("[See every change between your version and this update](%1) on GitHub.").arg(url) + qsl("\n\n");
}

void UpdateDialog::startDownload()
{
    mFeed->downloadRelease(mLatestRelease, /*requireChecksums=*/true);
    disableButtons(true);
}

void UpdateDialog::startUpdate()
{
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(mUpdateFilePath))) {
        done(QDialog::Accepted);
        QApplication::quit();
    } else {
        qWarning() << "Failed to open update file:" << mUpdateFilePath << "exists:" << QFile::exists(mUpdateFilePath);
        //: Error shown when the downloaded update file cannot be opened for installation. %1 is the file path.
        handleDownloadError(tr("Could not open the downloaded update. You can try opening it manually:\n%1").arg(mUpdateFilePath));
    }
}

void UpdateDialog::autoDownloadCheckboxToggled(bool enabled)
{
    enableAutoDownload(enabled, mSettings);
}

void UpdateDialog::handleFeedReady()
{
    mFeedLoadFailed = false;
    mUpdates = mFeed->getUpdates(dblsqd::Release::getCurrentRelease());
    mReleases = mFeed->getReleases();
    if (!mUpdates.isEmpty()) {
        mLatestRelease = mUpdates.first();
    }

    if (mType == ManualChangelog) {
        setupChangelogUi();
        emit ready();
        return;
    }

    mUpdateFilePath = settingsValue(qsl("updateFilePath"), "", mSettings).toString();
    if (!mUpdateFilePath.isEmpty() && QFile::exists(mUpdateFilePath)) {
        QString updateFileVersion = settingsValue(qsl("updateFileVersion"), "", mSettings).toString();
        if (updateFileVersion != mLatestRelease.getVersion() || updateFileVersion == QApplication::applicationVersion()) {
            if (!QFile::remove(mUpdateFilePath)) {
                qWarning() << "Failed to remove stale update file:" << mUpdateFilePath;
            }
            removeSetting(qsl("updateFilePath"), mSettings);
            removeSetting(qsl("updateFileVersion"), mSettings);
            mUpdateFilePath.clear();
        } else {
            mIsDownloadFinished = true;
        }
    }

    if (mUpdates.isEmpty()) {
        setupNoUpdatesUi();
        return;
    }

    QString latestVersion = mLatestRelease.getVersion();
    bool skipRelease = (settingsValue(qsl("skipRelease"), "", mSettings).toString() == latestVersion);
    bool autoDownload = autoDownloadEnabled(mSettings) && (!skipRelease);
    if (autoDownload && !mIsDownloadFinished && !mFeed->isDownloading()) {
        startDownload();
    }

    setupUpdateUi();
    emit ready();

    KDToolBox::connectSingleShot(mFeed, &Feed::ready, this, &UpdateDialog::handleFeedReady);
    KDToolBox::connectSingleShot(mFeed, &Feed::loadError, this, &UpdateDialog::handleLoadError);
}

void UpdateDialog::handleLoadError(const QString& message)
{
    qWarning() << "Update check failed:" << message;
    mFeedLoadFailed = true;
    if (isVisible()) {
        setupNoUpdatesUi();
        //: Label shown in the update dialog when the update check fails due to a network or server error
        mUi->labelHeadlineNoUpdates->setText(tr("Could not check for updates"));
        mUi->labelChangelog->setHtml(qsl("<p>%1</p>").arg(message.toHtmlEscaped()));
        mUi->labelChangelog->show();
        adjustDialogSize();
    }
    // Re-establish single-shot connections so the next feed load attempt reaches this dialog
    KDToolBox::connectSingleShot(mFeed, &Feed::ready, this, &UpdateDialog::handleFeedReady);
    KDToolBox::connectSingleShot(mFeed, &Feed::loadError, this, &UpdateDialog::handleLoadError);
}

void UpdateDialog::handleDownloadFinished()
{
    const QString filePath = mFeed->getDownloadFilePath();
    if (filePath.isEmpty()) {
        //: Error shown when the download finished but no file was saved
        handleDownloadError(tr("Download failed. Please try again."));
        return;
    }
    mIsDownloadFinished = true;
    mUpdateFilePath = filePath;
    setSettingsValue(qsl("updateFilePath"), mUpdateFilePath, mSettings);
    setSettingsValue(qsl("updateFileVersion"), mLatestRelease.getVersion(), mSettings);

    if (mAccepted) {
        if (mAcceptedInstallButton == nullptr) {
            startUpdate();
        } else {
            emit installButtonClicked(mAcceptedInstallButton, mUpdateFilePath);
        }

    } else {
        disableButtons(false);
    }
}

void UpdateDialog::handleDownloadError(const QString& message)
{
    //: Title for the download error warning dialog
    const QString errorTitle = tr("Download Error");
    //: Message shown in the download error warning dialog, followed by the specific error details
    QMessageBox::warning(this, errorTitle, tr("There was an error while downloading the update.") + qsl("\n\n") + message);
    done(QDialog::Rejected);
}

void UpdateDialog::updateProgressBar(qint64 bytesReceived, qint64 bytesTotal)
{
    mUi->progressBar->show();
    mUi->progressBar->setMaximum(bytesTotal / 1024);
    mUi->progressBar->setValue(bytesReceived / 1024);
}

void UpdateDialog::onLinkActivated(const QUrl& link)
{
    if (mOpenExternalLinks) {
        if (!QDesktopServices::openUrl(link)) {
            qWarning() << "Failed to open URL:" << link;
        }
    } else {
        emit linkActivated(link.toString());
    }
}

/*! \fn void UpdateDialog::ready()
 * This signal is emitted when the feed has been loaded and the UpdateDialog is
 * ready to be shown with show() or exec().
 * For ManualChangelog type, this is emitted regardless of whether updates are
 * available.
 */

/*! \fn void UpdateDialog::installButtonClicked(QAbstractButton* button, const QString&
 * filePath) This signal is emitted when a custom install button was clicked.
 */

} // namespace dblsqd
