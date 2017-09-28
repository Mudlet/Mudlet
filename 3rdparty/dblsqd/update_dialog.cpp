#include "update_dialog.h"
#include "ui_update_dialog.h"

namespace dblsqd {

/*!
 * \class UpdateDialog
 * \brief A dialog class for displaying and downloading update information.
 *
 * UpdateDialog is a drop-in class for adding a fully-functional auto-update
 * component to an existing application.
 *
 * The most simple integration is
 * possible with just three lines of code:
 * \code
 * dblsqd::Feed* feed = new dblsqd::Feed();
 * feed->setUrl("https://feeds.dblsqd.com/:app_token");
 * dblsqd::UpdateDialog* updateDialog = new dblsqd::UpdateDialog(feed);
 * \endcode
 *
 * The update dialog can also display an application icon which can be set with
 * setIcon().
 */

/*!
 * \enum UpdateDialog::Type
 * \brief This flag determines the when/if an UpdateDialog is displayed automatically.
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
 */

/*!
 * \brief Constructs a new UpdateDialog.
 *
 * A Feed object needs to be constructed first and passed to this constructor.
 * Feed::load() does not need to be called on the Feed object.
 *
 * The given UpdateDialog::Type flag determines when/if the dialog is shown
 * automatically.
 *
 * UpdateDialog uses QSettings to save information such as when a release was
 * skipped by the users. If you want to use a specially initialized QSettings
 * object, you may also pass it to this constructor.
 *
 */
UpdateDialog::UpdateDialog(Feed* feed, Type type, QWidget* parent, QSettings* settings) :
    QDialog(parent),
    ui(new Ui::UpdateDialog),
    feed(feed),
    type(type),
    settings(settings),
    settingsGroup("DBLSQD"),
    accepted(false),
    isDownloadFinished(false)
{
    ui->setupUi(this);
    toggleNoUpdates(true);

    ui->progressBar->setHidden(true);
    ui->labelIcon->setHidden(true);

    QString headline = ui->labelHeadlineNoUpdates->text();
    replaceAppVars(headline);
    ui->labelHeadlineNoUpdates->setText(headline);

    connect(feed, SIGNAL(ready()), this, SLOT(handleFeedReady()));
    connect(feed, SIGNAL(downloadFinished()), this, SLOT(handleDownloadFinished()));
    connect(feed, SIGNAL(downloadError(QString)), this, SLOT(handleDownloadError(QString)));
    connect(feed, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateProgressBar(qint64,qint64)));

    connect(ui->buttonConfirmNoUpdates, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->buttonConfirm, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->buttonSkip, SIGNAL(clicked()), this, SLOT(skip()));
    connect(ui->buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    ui->checkAutoDownload->setChecked(settingsValue("autoDownload", false).toBool());
    connect(ui->checkAutoDownload, SIGNAL(toggled(bool)), this, SLOT(toggleAutoDownloads(bool)));

    switch(type) {
        case Type::OnUpdateAvailable: {
            connect(this, SIGNAL(ready()), this, SLOT(showIfUpdatesAvailable()));
            break;
        }
        case Type::OnLastWindowClosed: {
            QGuiApplication* app = (QGuiApplication*) QApplication::instance();
            app->setQuitOnLastWindowClosed(false);
            connect(app, SIGNAL(lastWindowClosed()), this, SLOT(showIfUpdatesAvailable()));
            break;
        }
        case Type::Manual: {
            //don’t need to do anything
        }
    }
    feed->load();
}

UpdateDialog::~UpdateDialog()
{
    delete ui;
}


/*
 * Setters
 */
/*!
 * \brief Sets the icon displayed in the update window.
 */
void UpdateDialog::setIcon(QPixmap pixmap) {
    ui->labelIcon->setPixmap(QPixmap(pixmap));
    ui->labelIcon->setHidden(false);
}

void UpdateDialog::setIcon(QString fileName) {
    ui->labelIcon->setPixmap(QPixmap(fileName));
    ui->labelIcon->setHidden(false);
}


/*
 * Public Slots
 */
/*!
 * \brief Accepts the dialog.
 *
 * Accepting the UpdateDialog closes the dialog if no other action (such as
 * downloading or installing a Release) is required first.
 */
void UpdateDialog::accept() {
    accepted = true;
    if (isDownloadFinished) {
        startUpdate();
    } else if(!latestRelease.getVersion().isEmpty()) {
        startDownload();
    } else {
        done(QDialog::Accepted);
    }
}

/*!
 * \brief Rejects the dialog.
 */
void UpdateDialog::reject() {
    this->done(QDialog::Rejected);
}

/*!
 * \brief Skips the latest retrieved Release.
 *
 * If a release has been skipped, UpdateDialog will not be displayed
 * automatically when using Type::OnUpdateAvailable or
 * Type::OnLastWindowClosed.
 */
void UpdateDialog::skip() {
    if (!updateFilePath.isEmpty()) {
        QFile::remove(updateFilePath);

    }
    setSettingsValue("skipRelease", latestRelease.getVersion());
    done(QDialog::Rejected);
}


/*!
 * \brief Shows the dialog if there are available updates.
 */
void UpdateDialog::showIfUpdatesAvailable() {
    QString latestVersion = latestRelease.getVersion();
    bool skipRelease = (settingsValue("skipRelease").toString() == latestVersion);
    if (!latestVersion.isEmpty() && !skipRelease) {
        show();
    }
}


/*
 * Helpers
 */
QVariant UpdateDialog::settingsValue(QString key, QVariant defaultValue) {
    return settings->value(settingsGroup + "/" + key, defaultValue);
}

void UpdateDialog::setSettingsValue(QString key, QVariant value) {
    settings->setValue(settingsGroup + "/" + key, value);
}

void UpdateDialog::removeSetting(QString key) {
    settings->remove(settingsGroup + "/" + key);
}

void UpdateDialog::disableButtons(bool disable) {
    ui->buttonCancel->setDisabled(disable);
    ui->buttonConfirm->setDisabled(disable);
    ui->buttonSkip->setDisabled(disable);
    ui->checkAutoDownload->setDisabled(disable);
}

void UpdateDialog::toggleNoUpdates(bool noUpdates) {
    //Show these elements when there are no updates
    ui->headerContainerNoUpdates->setHidden(!noUpdates);
    ui->buttonConfirmNoUpdates->setHidden(!noUpdates);

    //Show these elements when there are updates
    ui->headerContainer->setHidden(noUpdates);
    ui->buttonSkip->setHidden(noUpdates);
    ui->scrollAreaChangelog->setHidden(noUpdates);
    ui->buttonCancel->setHidden(noUpdates);
    ui->buttonConfirm->setHidden(noUpdates);
    ui->buttonSkip->setHidden(noUpdates);

    //Focus the respective confirm button
    if (noUpdates) {
        ui->buttonConfirmNoUpdates->setFocus();
    } else {
        ui->buttonConfirm->setFocus();
    }
    this->adjustSize();
}

void UpdateDialog::replaceAppVars(QString &string) {
    string.replace("%APPNAME%", QCoreApplication::applicationName());
    string.replace("%CURRENT_VERSION%", QCoreApplication::applicationVersion());
    string.replace("%UPDATE_VERSION%", latestRelease.getVersion());
}

void UpdateDialog::startDownload() {
    feed->downloadRelease(latestRelease);
    disableButtons(true);
}

void UpdateDialog::startUpdate() {
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(updateFilePath))) {
        QApplication::quit();
    } else {
        handleDownloadError(tr("Could not open downloaded file %1").arg(updateFilePath));
    }
}


/*
 * Private Slots
 */

void UpdateDialog::toggleAutoDownloads(bool enable) {
    setSettingsValue("autoDownload", enable);
}

void UpdateDialog::handleFeedReady() {
    //Retrieve update information
    Release currentRelease(QApplication::applicationVersion());
    QList<Release> updates = feed->getUpdates(currentRelease);
    if (!updates.isEmpty()) {
        latestRelease = updates.first();
    }

    //Check if an update has been downloaded previously
    updateFilePath = settingsValue("updateFilePath").toString();
    if (!updateFilePath.isEmpty() && QFile::exists(updateFilePath)) {
        QString updateFileVersion = settingsValue("updateFileVersion").toString();
        if (updateFileVersion != latestRelease.getVersion() || updateFileVersion == QApplication::applicationVersion()) {
            QFile::remove(updateFilePath);
            removeSetting("updateFilePath");
            removeSetting("updateFileVersion");
            updateFilePath = "";
        } else {
            isDownloadFinished = true;
        }
    }

    //Check if there are any updates
    if (updates.isEmpty()) return;
    toggleNoUpdates(false);

    //Populate dialog header
    QString headline = ui->labelHeadline->text();
    replaceAppVars(headline);
    ui->labelHeadline->setText(headline);

    QString info = ui->labelInfo->text();
    replaceAppVars(info);
    ui->labelInfo->setText(info);

    //Populate changelog label
    QString changelog;
    for (int i = 0; i < updates.size(); i++) {
        QString h2Style = "font-size: medium;";
        if (i > 0) {
            h2Style.append("margin-top: 1em;");
        }
        changelog.append("<h2 style=\"" + h2Style + "\">" + updates.at(i).getVersion() + "</h2>");
        changelog.append("<p>" + updates.at(i).getChangelog() + "</p>");
    }
    ui->labelChangelog->setText(changelog);

    //Check if the current update has been skipped
    bool skipRelease = (settingsValue("skipRelease").toString() == latestRelease.getVersion());

    //Adapt buttons if release should be downloaded automatically or has been downloaded already
    if (isDownloadFinished) {
        ui->progressBar->show();
        ui->progressBar->setMaximum(1);
        ui->progressBar->setValue(1);
    }
    bool autoDownload = ui->checkAutoDownload->isChecked() && (!skipRelease);
    if (autoDownload && !isDownloadFinished) {
        startDownload();
    }
    if (isDownloadFinished || autoDownload) {
        ui->buttonConfirm->setText(tr("Install update now"));
        ui->buttonCancel->setText(tr("Install update later"));
    }

    emit ready();
}

void UpdateDialog::handleDownloadFinished() {
    QTemporaryFile* file = feed->getDownloadFile();
    isDownloadFinished = true;
    updateFilePath = file->fileName();
    file->setAutoRemove(false);
    file->close();
    file->deleteLater();
    setSettingsValue("updateFilePath", updateFilePath);
    setSettingsValue("updateFileVersion", latestRelease.getVersion());

    if (accepted) {
        startUpdate();
    } else {
        disableButtons(false);
    }
}

void UpdateDialog::handleDownloadError(QString message) {
    QErrorMessage* errorMessage = new QErrorMessage(this);
    errorMessage->showMessage(message);
    done(QDialog::Rejected);
}

void UpdateDialog::updateProgressBar(qint64 bytesReceived, qint64 bytesTotal) {
    ui->progressBar->show();
    ui->progressBar->setMaximum(bytesTotal / 1024);
    ui->progressBar->setValue(bytesReceived / 1024);
}



/*
 * Signals
 */
/*! \fn void Feed::ready()
 * This signal is emitted when a updates are available and the UpdateDialog is
 * ready to be shown with show() or exec().
 */

} // namespace dblsqd
