#include "UpdateDialog.h"
#include "ui_update_dialog.h"

#include "../utils.h"

#include <QAbstractButton>
#include <QAction>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QTemporaryFile>
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
, mUi(new Ui::UpdateDialog)
, mFeed(feed)
, mType(type)
, mSettings(settings)
, mAccepted(false)
, mIsDownloadFinished(false)
, mAcceptedInstallButton(nullptr)
{
    mUi->setupUi(this);

    mUi->buttonCancel->addAction(mUi->actionCancel);
    mUi->buttonCancel->addAction(mUi->actionSkip);
    mUi->buttonCancel->setDefaultAction(mUi->actionCancel);

    mOpenExternalLinks = true;
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
        connect(mFeed, &Feed::ready, this, &UpdateDialog::handleFeedReady);
        connect(mFeed, &Feed::loadError, this, &UpdateDialog::handleLoadError);
    }
}

UpdateDialog::~UpdateDialog()
{
    delete mUi;
}

/*!
 * \brief Sets the icon displayed in the update window.
 */
void UpdateDialog::setIcon(QPixmap pixmap)
{
    mUi->labelIcon->setPixmap(QPixmap(pixmap));
    mUi->labelIcon->setHidden(false);
}

void UpdateDialog::setIcon(QString fileName)
{
    mUi->labelIcon->setPixmap(QPixmap(fileName));
    mUi->labelIcon->setHidden(false);
}

/*!
 * \brief Sets the minimum version to be displayed in the changelog.
 * Defaults to QApplication::applicationVersion() if not set.
 * \param version
 */
void UpdateDialog::setMinVersion(QString version)
{
    mMinVersion = version;
    setupChangelogUi();
}

/*!
 * \brief Sets the maximum version to be displayed in the changelog
 * \param version
 */
void UpdateDialog::setMaxVersion(QString version)
{
    mMaxVersion = version;
    setupChangelogUi();
}

/*!
 * \brief Convenience method for setting minimum and maximum version to be
 * displayed in the changelog. maximumVersion is set to
 * QApplication::applicationVersion() \param previousVersion
 */
void UpdateDialog::setPreviousVersion(QString previousVersion)
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
bool UpdateDialog::openExternalLinks()
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
        QFile::remove(mUpdateFilePath);
    }
    setSettingsValue("skipRelease", mLatestRelease.getVersion(), mSettings);
    done(QDialog::Rejected);
}

/*!
 * \brief Shows the dialog if there are available updates.
 */
void UpdateDialog::showIfUpdatesAvailable()
{
    QString latestVersion = mLatestRelease.getVersion();
    bool skipRelease = (settingsValue("skipRelease", "", mSettings).toString() == latestVersion);
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
    bool skipRelease = (settingsValue("skipRelease", "", mSettings).toString() == latestVersion);
    if (!latestVersion.isEmpty() && !skipRelease) {
        show();
    } else {
        QCoreApplication::quit();
    }
}

QVariant UpdateDialog::settingsValue(const QString& key, const QVariant& defaultValue, QSettings* settings)
{
    return settings->value("DBLSQD/" + key, defaultValue);
}

void UpdateDialog::setSettingsValue(const QString& key, const QVariant& value, QSettings* settings)
{
    settings->setValue("DBLSQD/" + key, value);
}

void UpdateDialog::removeSetting(const QString& key, QSettings* settings)
{
    settings->remove("DBLSQD/" + key);
}

void UpdateDialog::setDefaultSettingsValue(const QString& key, const QVariant& value, QSettings* settings)
{
    if (settings->contains("DBLSQD/" + key))
        return;
    setSettingsValue(key, value, settings);
}

/*!
 * \brief Enables or disables automatic downloads.
 */
void UpdateDialog::enableAutoDownload(bool enabled, QSettings* settings)
{
    setSettingsValue("autoDownload", enabled, settings);
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
        setDefaultSettingsValue("autoDownload", defaultValue, settings);
    } else {
        defaultValue = false;
    }
    return settingsValue("autoDownload", defaultValue, settings).toBool();
}

/*!
 * \overload
 */
bool UpdateDialog::autoDownloadEnabled(QSettings* settings)
{
    return settingsValue("autoDownload", false, settings).toBool();
}

void UpdateDialog::adjustDialogSize()
{
    adjustSize();

/*HACK: Qt seems to incorrectly calculate window geometry on Windows.
        This code avoids warning messages logged by the application
        in that case.*/
#if defined(Q_OS_WIN)
    QSize dialogSize = size();
    resize(dialogSize.width(), dialogSize.height() + 3);
#endif
}

void UpdateDialog::resetUi()
{
    QList<QWidget*> hiddenWidgets;
    for (int i = 0; i < mInstallButtons.size(); i++) {
        hiddenWidgets << mInstallButtons.at(i);
    }
    hiddenWidgets << mUi->headerContainer << mUi->labelIcon << mUi->headerContainerLoading << mUi->headerContainerNoUpdates << mUi->headerContainerChangelog << mUi->labelChangelog << mUi->progressBar
                  << mUi->checkAutoDownload << mUi->buttonCancel << mUi->buttonCancelLoading << mUi->buttonConfirm << mUi->buttonInstall;
    for (int i = 0; i < hiddenWidgets.size(); i++) {
        hiddenWidgets.at(i)->hide();
        hiddenWidgets.at(i)->disconnect();
    }
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
    for (int i = 0; i < showWidgets.size(); i++) {
        showWidgets.at(i)->show();
    }

    QList<QLabel*> labels;
    labels << mUi->labelHeadline << mUi->labelInfo;
    for (int i = 0; i < labels.size(); i++) {
        QString text = labels.at(i)->text();
        replaceAppVars(text);
        labels.at(i)->setText(text);
    }
    mUi->labelChangelog->setHtml(generateChangelogDocument());

    mUi->checkAutoDownload->setChecked(autoDownloadEnabled(mSettings));

    auto title = windowTitle();
    replaceAppVars(title);
    setWindowTitle(title);

    // Adapt buttons if release has been downloaded already
    if (mIsDownloadFinished) {
        mUi->progressBar->show();
        mUi->progressBar->setMaximum(1);
        mUi->progressBar->setValue(1);
    }

    connect(mFeed, &Feed::downloadFinished, this, &UpdateDialog::handleDownloadFinished);
    connect(mFeed, &Feed::downloadError, this, &UpdateDialog::handleDownloadError);
    connect(mFeed, &Feed::downloadProgress, this, &UpdateDialog::updateProgressBar);

    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    connect(mUi->actionCancel, &QAction::triggered, this, &UpdateDialog::reject);
    connect(mUi->actionSkip, &QAction::triggered, this, &UpdateDialog::skip);
    connect(mUi->checkAutoDownload, &QCheckBox::toggled, this, &UpdateDialog::autoDownloadCheckboxToggled);

    // Install buttons
    if (mInstallButtons.isEmpty()) {
        mUi->buttonInstall->setFocus();
        connect(mUi->buttonInstall, &QPushButton::clicked, this, &UpdateDialog::onButtonInstall);
    } else {
        mUi->buttonInstall->hide();
        for (int i = 0; i < mInstallButtons.size(); i++) {
            mInstallButtons.at(i)->show();
            connect(mInstallButtons.at(i), &QAbstractButton::clicked, this, &UpdateDialog::onButtonCustomInstall);
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
    for (int i = 0; i < showWidgets.size(); i++) {
        showWidgets.at(i)->show();
    }
    QList<QLabel*> labels;
    labels << mUi->labelHeadlineChangelog << mUi->labelInfoChangelog;
    for (int i = 0; i < labels.size(); i++) {
        QString text = labels.at(i)->text();
        replaceAppVars(text);
        labels.at(i)->setText(text);
    }

    auto title = windowTitle();
    replaceAppVars(title);
    setWindowTitle(title);

    mUi->labelChangelog->setHtml(generateChangelogDocument());
    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    mUi->buttonConfirm->setFocus();
    adjustDialogSize();
}

void UpdateDialog::setupNoUpdatesUi()
{
    resetUi();
    QList<QWidget*> showWidgets;
    showWidgets << mUi->headerContainerNoUpdates << mUi->buttonConfirm;
    for (int i = 0; i < showWidgets.size(); i++) {
        showWidgets.at(i)->show();
    }
    mUi->buttonConfirm->setFocus();

    QString text = mUi->labelHeadlineNoUpdates->text();
    replaceAppVars(text);
    mUi->labelHeadlineNoUpdates->setText(text);

    auto title = windowTitle();
    replaceAppVars(title);
    setWindowTitle(title);

    connect(mUi->buttonConfirm, &QPushButton::clicked, this, &UpdateDialog::accept);
    adjustDialogSize();
}

void UpdateDialog::disableButtons(bool disable)
{
    QList<QWidget*> buttons;
    for (int i = 0; i < mInstallButtons.size(); i++) {
        buttons << mInstallButtons.at(i);
    }
    buttons << mUi->buttonCancel << mUi->buttonCancelLoading << mUi->buttonConfirm << mUi->buttonConfirm << mUi->buttonInstall << mUi->checkAutoDownload;
    for (int i = 0; i < buttons.size(); i++) {
        buttons.at(i)->setDisabled(disable);
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
    } else {
        Release minRelease(mMinVersion.isEmpty() ? QApplication::applicationVersion() : mMinVersion);
        Release maxRelease(mMaxVersion);
        for (int i = 0; i < mReleases.size(); i++) {
            if (minRelease < mReleases.at(i) && (mMaxVersion.isEmpty() || mReleases.at(i) <= maxRelease)) {
                changelogReleases << mReleases.at(i);
            }
        }
    }
    for (int i = 0; i < changelogReleases.size(); i++) {
        QString h2Style = "font-size: medium;";
        if (i > 0) {
            h2Style.append("margin-top: 1em;");
        }
        changelog.append("<h2 style=\"" + h2Style + "\">" + changelogReleases.at(i).getVersion() + "</h2>");
        changelog.append("<p>" + changelogReleases.at(i).getChangelog() + "</p>");
    }
    return changelog;
}

void UpdateDialog::startDownload()
{
    mFeed->downloadRelease(mLatestRelease);
    disableButtons(true);
}

void UpdateDialog::startUpdate()
{
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(mUpdateFilePath))) {
        done(QDialog::Accepted);
        QApplication::quit();
    } else {
        handleDownloadError(tr("Could not open downloaded file %1").arg(mUpdateFilePath));
    }
}

void UpdateDialog::autoDownloadCheckboxToggled(bool enabled)
{
    enableAutoDownload(enabled, mSettings);
}

void UpdateDialog::handleFeedReady()
{
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

    // Check if an update has been downloaded previously
    mUpdateFilePath = settingsValue("updateFilePath", "", mSettings).toString();
    if (!mUpdateFilePath.isEmpty() && QFile::exists(mUpdateFilePath)) {
        QString updateFileVersion = settingsValue("updateFileVersion", "", mSettings).toString();
        if (updateFileVersion != mLatestRelease.getVersion() || updateFileVersion == QApplication::applicationVersion()) {
            QFile::remove(mUpdateFilePath);
            removeSetting("updateFilePath", mSettings);
            removeSetting("updateFileVersion", mSettings);
            mUpdateFilePath = "";
        } else {
            mIsDownloadFinished = true;
        }
    }

    // Check if there are any updates
    if (mUpdates.isEmpty()) {
        setupNoUpdatesUi();
        return;
    }

    // Automatic downloads
    QString latestVersion = mLatestRelease.getVersion();
    bool skipRelease = (settingsValue("skipRelease", "", mSettings).toString() == latestVersion);
    bool autoDownload = autoDownloadEnabled(mSettings) && (!skipRelease);
    if (autoDownload && !mIsDownloadFinished) {
        startDownload();
    }

    // Setup UI
    setupUpdateUi();
    emit ready();
}

void UpdateDialog::handleLoadError(const QString& message)
{
    qWarning() << "Update check failed:" << message;
    if (isVisible()) {
        setupNoUpdatesUi();
        //: Label shown when update check fails
        mUi->labelHeadlineNoUpdates->setText(tr("Could not check for updates"));
    }
}

void UpdateDialog::handleDownloadFinished()
{
    QTemporaryFile* file = mFeed->getDownloadFile();
    if (!file) {
        handleDownloadError(tr("Download completed but no file available"));
        return;
    }
    mIsDownloadFinished = true;
    mUpdateFilePath = file->fileName();
    file->setAutoRemove(false);
    file->close();
    file->deleteLater();
    setSettingsValue("updateFilePath", mUpdateFilePath, mSettings);
    setSettingsValue("updateFileVersion", mLatestRelease.getVersion(), mSettings);

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
    QMessageBox::warning(this, tr("Download Error"), tr("There was an error while downloading the update.") + qsl("\n\n") + message);
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
        QDesktopServices::openUrl(link);
    } else {
        emit linkActivated(link.toString());
    }
}

/*! \fn void UpdateDialog::ready()
 * This signal is emitted when updates are available and the UpdateDialog is
 * ready to be shown with show() or exec().
 */

/*! \fn void UpdateDialog::installButtonClicked(QAbstractButton* button, QString
 * filePath) This signal is emitted when a custom install button was clicked.
 */

} // namespace dblsqd
