/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "dlgSpeechRecognitionSetup.h"

#include "mudlet.h"
#include "VoskRecognizer.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

dlgSpeechRecognitionSetup::dlgSpeechRecognitionSetup(QWidget* parent)
: QDialog(parent)
{
    setWindowTitle(tr("Speech Recognition Setup"));
    setMinimumSize(550, 450);
    resize(600, 500);

    mpNetworkManager = new QNetworkAccessManager(this);

    setupUi();
    populateModelList();
    updateNavigationButtons();

    // Check library status on startup
    mVoskLibraryAvailable = checkVoskLibrary();
}

dlgSpeechRecognitionSetup::~dlgSpeechRecognitionSetup()
{
    if (mpCurrentDownload) {
        mpCurrentDownload->abort();
        mpCurrentDownload->deleteLater();
    }
}

void dlgSpeechRecognitionSetup::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Create stacked widget for wizard pages
    mpStackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(mpStackedWidget, 1);

    // Setup individual pages
    setupWelcomePage();
    setupLibraryPage();
    setupModelSelectionPage();
    setupDownloadPage();
    setupCompletionPage();

    // Navigation buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    mpBackButton = new QPushButton(tr("&Back"), this);
    mpBackButton->setEnabled(false);
    mpBackButton->setAccessibleName(tr("Back"));
    mpBackButton->setAccessibleDescription(tr("Go back to the previous step in the speech recognition setup wizard."));
    connect(mpBackButton, &QPushButton::clicked, this, &dlgSpeechRecognitionSetup::slot_previousPage);
    buttonLayout->addWidget(mpBackButton);

    mpNextButton = new QPushButton(tr("&Next"), this);
    mpNextButton->setDefault(true);
    mpNextButton->setAccessibleName(tr("Next"));
    mpNextButton->setAccessibleDescription(tr("Proceed to the next step in the speech recognition setup wizard."));
    connect(mpNextButton, &QPushButton::clicked, this, &dlgSpeechRecognitionSetup::slot_nextPage);
    buttonLayout->addWidget(mpNextButton);

    mpCancelButton = new QPushButton(tr("&Cancel"), this);
    mpCancelButton->setAccessibleName(tr("Cancel"));
    mpCancelButton->setAccessibleDescription(tr("Cancel the speech recognition setup and close this wizard."));
    connect(mpCancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(mpCancelButton);

    mainLayout->addLayout(buttonLayout);
}

void dlgSpeechRecognitionSetup::setupWelcomePage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* titleLabel = new QLabel(tr("<h2>Welcome to Speech Recognition Setup</h2>"), page);
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    mpWelcomeLabel = new QLabel(page);
    mpWelcomeLabel->setWordWrap(true);
    mpWelcomeLabel->setText(tr("<p>This wizard will help you set up speech-to-text functionality for Mudlet.</p>"
                               "<p>Speech recognition allows you to speak commands instead of typing them, "
                               "which can be helpful for accessibility or hands-free gaming.</p>"
                               "<p><b>What you'll need:</b></p>"
                               "<ul>"
                               "<li>The Vosk speech recognition library installed on your system</li>"
                               "<li>A language model for your preferred language (~40-50 MB for small models)</li>"
                               "<li>A working microphone</li>"
                               "</ul>"
                               "<p>Click <b>Next</b> to check your system and begin setup.</p>"));
    layout->addWidget(mpWelcomeLabel);

    layout->addStretch();
    mpStackedWidget->addWidget(page);
}

void dlgSpeechRecognitionSetup::setupLibraryPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* titleLabel = new QLabel(tr("<h2>Vosk Library Status</h2>"), page);
    layout->addWidget(titleLabel);

    // Status display
    auto* statusLayout = new QHBoxLayout();
    mpLibraryStatusIcon = new QLabel(page);
    mpLibraryStatusIcon->setFixedSize(32, 32);
    statusLayout->addWidget(mpLibraryStatusIcon);

    mpLibraryStatusLabel = new QLabel(page);
    mpLibraryStatusLabel->setWordWrap(true);
    mpLibraryStatusLabel->setAccessibleName(tr("Vosk library status"));
    mpLibraryStatusLabel->setAccessibleDescription(tr("Status information about whether the Vosk speech recognition library is installed and available."));
    statusLayout->addWidget(mpLibraryStatusLabel, 1);
    layout->addLayout(statusLayout);

    // Instructions
    mpLibraryInstructions = new QTextEdit(page);
    mpLibraryInstructions->setReadOnly(true);
    mpLibraryInstructions->setMinimumHeight(120);
    mpLibraryInstructions->setAccessibleName(tr("Installation instructions"));
    mpLibraryInstructions->setAccessibleDescription(tr("Detailed instructions for installing the Vosk speech recognition library on your system."));
    layout->addWidget(mpLibraryInstructions);

    // Download progress (initially hidden)
    mpLibraryDownloadStatusLabel = new QLabel(page);
    mpLibraryDownloadStatusLabel->setWordWrap(true);
    mpLibraryDownloadStatusLabel->setVisible(false);
    layout->addWidget(mpLibraryDownloadStatusLabel);

    mpLibraryDownloadProgressBar = new QProgressBar(page);
    mpLibraryDownloadProgressBar->setRange(0, 100);
    mpLibraryDownloadProgressBar->setValue(0);
    mpLibraryDownloadProgressBar->setVisible(false);
    mpLibraryDownloadProgressBar->setAccessibleName(tr("Library download progress"));
    mpLibraryDownloadProgressBar->setAccessibleDescription(tr("Shows the progress of downloading the Vosk speech recognition library."));
    layout->addWidget(mpLibraryDownloadProgressBar);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();

    mpDownloadLibraryButton = new QPushButton(tr("Download && Install Vosk Library"), page);
    mpDownloadLibraryButton->setToolTip(tr("Automatically download and install the Vosk library for your platform"));
    mpDownloadLibraryButton->setAccessibleName(tr("Download and install Vosk library"));
    mpDownloadLibraryButton->setAccessibleDescription(tr("Automatically download and install the Vosk speech recognition library for your platform."));
    connect(mpDownloadLibraryButton, &QPushButton::clicked, this, &dlgSpeechRecognitionSetup::slot_downloadLibrary);
    buttonLayout->addWidget(mpDownloadLibraryButton);

    mpOpenVoskWebsiteButton = new QPushButton(tr("Manual Install..."), page);
    mpOpenVoskWebsiteButton->setToolTip(tr("Open the Vosk website for manual installation instructions"));
    mpOpenVoskWebsiteButton->setAccessibleName(tr("Manual install"));
    mpOpenVoskWebsiteButton->setAccessibleDescription(tr("Open the Vosk website for manual installation instructions."));
    connect(mpOpenVoskWebsiteButton, &QPushButton::clicked, this, &dlgSpeechRecognitionSetup::slot_openVoskWebsite);
    buttonLayout->addWidget(mpOpenVoskWebsiteButton);

    mpRefreshLibraryButton = new QPushButton(tr("Refresh Status"), page);
    mpRefreshLibraryButton->setAccessibleName(tr("Refresh library status"));
    mpRefreshLibraryButton->setAccessibleDescription(tr("Check again if the Vosk library is installed and available."));
    connect(mpRefreshLibraryButton, &QPushButton::clicked, this, &dlgSpeechRecognitionSetup::slot_refreshLibraryStatus);
    buttonLayout->addWidget(mpRefreshLibraryButton);

    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    layout->addStretch();
    mpStackedWidget->addWidget(page);
}

void dlgSpeechRecognitionSetup::setupModelSelectionPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* titleLabel = new QLabel(tr("<h2>Select Language Model</h2>"), page);
    layout->addWidget(titleLabel);

    auto* introLabel = new QLabel(tr("<p>Choose a language model for speech recognition. "
                                     "Small models are faster and use less memory, while larger models are more accurate.</p>"),
                                  page);
    introLabel->setWordWrap(true);
    layout->addWidget(introLabel);

    // Model selection
    auto* modelGroup = new QGroupBox(tr("Available Models"), page);
    auto* modelLayout = new QGridLayout(modelGroup);

    modelLayout->addWidget(new QLabel(tr("Language Model:")), 0, 0);
    mpModelComboBox = new QComboBox(page);
    mpModelComboBox->setMinimumWidth(300);
    mpModelComboBox->setAccessibleName(tr("Language model"));
    mpModelComboBox->setAccessibleDescription(tr("Select the language model to download for speech recognition. Small models are faster and use less memory, while larger models are more accurate."));
    connect(mpModelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && index < mAvailableModels.size()) {
            const auto& model = mAvailableModels[index];
            mpModelDescriptionLabel->setText(model.isSmall ? tr("Lightweight model - faster, uses less memory") : tr("Full model - more accurate, requires more resources"));

            // Check if this model is already downloaded
            QStringList installedModels = VoskRecognizer::getInstalledModels();
            bool isInstalled = installedModels.contains(model.identifier);

            if (isInstalled) {
                mpModelSizeLabel->setText(tr("Already downloaded"));
            } else {
                mpModelSizeLabel->setText(tr("Download size: %1").arg(formatSize(model.sizeBytes)));
            }

            // Update button text based on whether model is downloaded
            updateNavigationButtons();
        }
    });
    modelLayout->addWidget(mpModelComboBox, 0, 1);

    mpModelDescriptionLabel = new QLabel(page);
    mpModelDescriptionLabel->setWordWrap(true);
    mpModelDescriptionLabel->setAccessibleName(tr("Model description"));
    mpModelDescriptionLabel->setAccessibleDescription(tr("Description of the selected language model."));
    modelLayout->addWidget(mpModelDescriptionLabel, 1, 0, 1, 2);

    mpModelSizeLabel = new QLabel(page);
    modelLayout->addWidget(mpModelSizeLabel, 2, 0, 1, 2);

    layout->addWidget(modelGroup);
    layout->addStretch();
    mpStackedWidget->addWidget(page);
}

void dlgSpeechRecognitionSetup::setupDownloadPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* titleLabel = new QLabel(tr("<h2>Downloading Model</h2>"), page);
    layout->addWidget(titleLabel);

    mpDownloadStatusLabel = new QLabel(tr("Preparing download..."), page);
    mpDownloadStatusLabel->setWordWrap(true);
    layout->addWidget(mpDownloadStatusLabel);

    mpDownloadProgressBar = new QProgressBar(page);
    mpDownloadProgressBar->setRange(0, 100);
    mpDownloadProgressBar->setValue(0);
    mpDownloadProgressBar->setAccessibleName(tr("Model download progress"));
    mpDownloadProgressBar->setAccessibleDescription(tr("Shows the progress of downloading the selected language model."));
    layout->addWidget(mpDownloadProgressBar);

    mpDownloadDetailsLabel = new QLabel(page);
    mpDownloadDetailsLabel->setWordWrap(true);
    layout->addWidget(mpDownloadDetailsLabel);

    layout->addStretch();
    mpStackedWidget->addWidget(page);
}

void dlgSpeechRecognitionSetup::setupCompletionPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* titleLabel = new QLabel(tr("<h2>Setup Complete</h2>"), page);
    layout->addWidget(titleLabel);

    auto* statusLayout = new QHBoxLayout();
    mpCompletionIcon = new QLabel(page);
    mpCompletionIcon->setFixedSize(48, 48);
    statusLayout->addWidget(mpCompletionIcon);

    mpCompletionLabel = new QLabel(page);
    mpCompletionLabel->setWordWrap(true);
    mpCompletionLabel->setAccessibleName(tr("Setup completion status"));
    mpCompletionLabel->setAccessibleDescription(tr("Status message indicating whether the speech recognition setup was completed successfully."));
    statusLayout->addWidget(mpCompletionLabel, 1);
    layout->addLayout(statusLayout);

    mpCompletionDetailsLabel = new QLabel(page);
    mpCompletionDetailsLabel->setWordWrap(true);
    layout->addWidget(mpCompletionDetailsLabel);

    layout->addStretch();
    mpStackedWidget->addWidget(page);
}

void dlgSpeechRecognitionSetup::populateModelList()
{
    // Populate with common Vosk models
    // These are the official models from https://alphacephei.com/vosk/models
    mAvailableModels.clear();

    // English models
    mAvailableModels.append(
            {tr("English (US) - Small"), qsl("vosk-model-small-en-us-0.15"), qsl("en"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip"), 40 * 1024 * 1024, true});
    mAvailableModels.append({tr("English (US) - Large"), qsl("vosk-model-en-us-0.22"), qsl("en"), qsl("https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip"), 1800 * 1024 * 1024, false});

    // German models
    mAvailableModels.append({tr("German - Small"), qsl("vosk-model-small-de-0.15"), qsl("de"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-de-0.15.zip"), 45 * 1024 * 1024, true});
    mAvailableModels.append({tr("German - Large"), qsl("vosk-model-de-0.21"), qsl("de"), qsl("https://alphacephei.com/vosk/models/vosk-model-de-0.21.zip"), 1900 * 1024 * 1024, false});

    // Spanish models
    mAvailableModels.append({tr("Spanish - Small"), qsl("vosk-model-small-es-0.42"), qsl("es"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-es-0.42.zip"), 39 * 1024 * 1024, true});

    // French models
    mAvailableModels.append({tr("French - Small"), qsl("vosk-model-small-fr-0.22"), qsl("fr"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-fr-0.22.zip"), 41 * 1024 * 1024, true});
    mAvailableModels.append({tr("French - Large"), qsl("vosk-model-fr-0.22"), qsl("fr"), qsl("https://alphacephei.com/vosk/models/vosk-model-fr-0.22.zip"), 1400 * 1024 * 1024, false});

    // Italian models
    mAvailableModels.append({tr("Italian - Small"), qsl("vosk-model-small-it-0.22"), qsl("it"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-it-0.22.zip"), 48 * 1024 * 1024, true});

    // Portuguese models
    mAvailableModels.append({tr("Portuguese - Small"), qsl("vosk-model-small-pt-0.3"), qsl("pt"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-pt-0.3.zip"), 31 * 1024 * 1024, true});

    // Russian models
    mAvailableModels.append({tr("Russian - Small"), qsl("vosk-model-small-ru-0.22"), qsl("ru"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-ru-0.22.zip"), 45 * 1024 * 1024, true});

    // Chinese models
    mAvailableModels.append({tr("Chinese - Small"), qsl("vosk-model-small-cn-0.22"), qsl("zh"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip"), 42 * 1024 * 1024, true});

    // Japanese models
    mAvailableModels.append({tr("Japanese - Small"), qsl("vosk-model-small-ja-0.22"), qsl("ja"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-ja-0.22.zip"), 48 * 1024 * 1024, true});

    // Polish models
    mAvailableModels.append({tr("Polish - Small"), qsl("vosk-model-small-pl-0.22"), qsl("pl"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-pl-0.22.zip"), 47 * 1024 * 1024, true});

    // Dutch models
    mAvailableModels.append({tr("Dutch - Small"), qsl("vosk-model-small-nl-0.22"), qsl("nl"), qsl("https://alphacephei.com/vosk/models/vosk-model-small-nl-0.22.zip"), 39 * 1024 * 1024, true});

    // Populate combo box
    mpModelComboBox->clear();
    for (const auto& model : mAvailableModels) {
        QString displayText = model.name + tr(" (~%1)").arg(formatSize(model.sizeBytes));
        mpModelComboBox->addItem(displayText);
    }

    // Check for currently selected/used model from settings
    QString currentModelPath = VoskRecognizer::getSelectedModelPath();
    int selectedIndex = -1;

    if (!currentModelPath.isEmpty()) {
        // Extract the model identifier from the path
        QDir modelDir(currentModelPath);
        QString currentModelId = modelDir.dirName();

        // Find this model in our list
        for (int i = 0; i < mAvailableModels.size(); ++i) {
            if (mAvailableModels[i].identifier == currentModelId) {
                selectedIndex = i;
                break;
            }
        }
    }

    // If no current model found, fall back to small model matching interface language
    if (selectedIndex < 0) {
        QString defaultLang = getDefaultLanguageCode();
        for (int i = 0; i < mAvailableModels.size(); ++i) {
            if (mAvailableModels[i].languageCode == defaultLang && mAvailableModels[i].isSmall) {
                selectedIndex = i;
                break;
            }
        }
    }

    // Default to first model if nothing matched
    if (selectedIndex < 0) {
        selectedIndex = 0;
    }

    mpModelComboBox->setCurrentIndex(selectedIndex);
}

QString dlgSpeechRecognitionSetup::getDefaultLanguageCode() const
{
    // Get Mudlet's interface language or fall back to system locale
    QString lang = QLocale::system().name().left(2);
    return lang;
}

bool dlgSpeechRecognitionSetup::checkVoskLibrary()
{
    bool available = VoskRecognizer::isLibraryAvailable();

    if (available) {
        mpLibraryStatusIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(32, 32));
        mpLibraryStatusLabel->setText(tr("<b style='color: green;'>Vosk library is installed and ready!</b>"));
        mpLibraryInstructions->setHtml(tr("<p>The Vosk speech recognition library was found on your system.</p>"
                                          "<p>You can proceed to select a language model for speech recognition.</p>"));
        mpDownloadLibraryButton->setVisible(false);
        mpOpenVoskWebsiteButton->setVisible(false);
        mpLibraryDownloadProgressBar->setVisible(false);
        mpLibraryDownloadStatusLabel->setVisible(false);
    } else {
        mpLibraryStatusIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32));
        mpLibraryStatusLabel->setText(tr("<b style='color: orange;'>Vosk library not found</b>"));

        LibraryInfo libInfo = getLibraryInfoForCurrentPlatform();
        QString platformName;
        QString installLocation;

#if defined(Q_OS_MACOS)
        platformName = tr("macOS");
        installLocation = QCoreApplication::applicationDirPath() + qsl("/../Frameworks/");
#elif defined(Q_OS_WIN)
        platformName = tr("Windows");
        installLocation = QCoreApplication::applicationDirPath();
#else
        platformName = tr("Linux");
        installLocation = qsl("/usr/local/lib/");
#endif

        mpLibraryInstructions->setHtml(tr("<p>The Vosk library needs to be installed before you can use speech recognition.</p>"
                                          "<p><b>Recommended:</b> Click the <b>Download & Install</b> button below to automatically "
                                          "download and install the Vosk library (~%1) for %2.</p>"
                                          "<p>The library will be installed to:<br/><code>%3</code></p>"
                                          "<p>Alternatively, click <b>Manual Install</b> for more options.</p>")
                                               .arg(formatSize(libInfo.sizeBytes), platformName, installLocation));

        mpDownloadLibraryButton->setVisible(true);
        mpDownloadLibraryButton->setEnabled(!mDownloadingLibrary);
        mpOpenVoskWebsiteButton->setVisible(true);
    }

    return available;
}

void dlgSpeechRecognitionSetup::updateNavigationButtons()
{
    Page currentPage = static_cast<Page>(mpStackedWidget->currentIndex());

    mpBackButton->setEnabled(currentPage > WelcomePage && currentPage != DownloadPage);
    mpCancelButton->setEnabled(currentPage != DownloadPage || !mpCurrentDownload);

    switch (currentPage) {
    case WelcomePage:
        mpNextButton->setText(tr("&Next"));
        mpNextButton->setEnabled(true);
        break;
    case LibraryPage:
        mpNextButton->setText(tr("&Next"));
        mpNextButton->setEnabled(mVoskLibraryAvailable);
        break;
    case ModelSelectionPage: {
        // Check if the currently selected model is already downloaded
        int index = mpModelComboBox->currentIndex();
        bool isInstalled = false;
        if (index >= 0 && index < mAvailableModels.size()) {
            QStringList installedModels = VoskRecognizer::getInstalledModels();
            isInstalled = installedModels.contains(mAvailableModels[index].identifier);
        }
        mpNextButton->setText(isInstalled ? tr("&Select") : tr("&Download"));
        mpNextButton->setEnabled(true);
        break;
    }
    case DownloadPage:
        mpNextButton->setText(tr("&Next"));
        mpNextButton->setEnabled(false);
        break;
    case CompletionPage:
        mpNextButton->setText(tr("&Finish"));
        mpNextButton->setEnabled(true);
        break;
    }
}

void dlgSpeechRecognitionSetup::goToPage(Page page)
{
    mpStackedWidget->setCurrentIndex(page);
    updateNavigationButtons();
}

void dlgSpeechRecognitionSetup::slot_nextPage()
{
    Page currentPage = static_cast<Page>(mpStackedWidget->currentIndex());

    switch (currentPage) {
    case WelcomePage:
        goToPage(LibraryPage);
        slot_refreshLibraryStatus();
        break;
    case LibraryPage:
        if (mVoskLibraryAvailable) {
            goToPage(ModelSelectionPage);
        }
        break;
    case ModelSelectionPage: {
        // Check if the model is already downloaded
        int index = mpModelComboBox->currentIndex();
        if (index >= 0 && index < mAvailableModels.size()) {
            const auto& model = mAvailableModels[index];
            QStringList installedModels = VoskRecognizer::getInstalledModels();

            if (installedModels.contains(model.identifier)) {
                // Model is already downloaded - just select it and skip to completion
                QString modelsDir = VoskRecognizer::modelsDirectoryPath();
                QString modelPath = modelsDir + QDir::separator() + model.identifier;
                mInstalledModelPath = modelPath;
                VoskRecognizer::setSelectedModelPath(modelPath);
                mSetupCompleted = true;

                mpCompletionIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(48, 48));
                mpCompletionLabel->setText(tr("<b>Model selected!</b>"));
                mpCompletionDetailsLabel->setText(tr("<p>The speech recognition model is ready:</p>"
                                                     "<p><code>%1</code></p>"
                                                     "<p>You can now use the microphone button in the command line to speak commands.</p>")
                                                          .arg(mInstalledModelPath));

                goToPage(CompletionPage);
            } else {
                // Model needs to be downloaded
                goToPage(DownloadPage);
                slot_downloadModel();
            }
        }
        break;
    }
    case DownloadPage:
        goToPage(CompletionPage);
        break;
    case CompletionPage:
        mSetupCompleted = true;
        accept();
        break;
    }
}

void dlgSpeechRecognitionSetup::slot_previousPage()
{
    Page currentPage = static_cast<Page>(mpStackedWidget->currentIndex());

    if (currentPage > WelcomePage && currentPage != DownloadPage) {
        goToPage(static_cast<Page>(currentPage - 1));
    }
}

void dlgSpeechRecognitionSetup::slot_downloadModel()
{
    int index = mpModelComboBox->currentIndex();
    if (index < 0 || index >= mAvailableModels.size()) {
        mpDownloadStatusLabel->setText(tr("Error: No model selected"));
        return;
    }

    const auto& model = mAvailableModels[index];
    mCurrentlyDownloadingModelId = model.identifier; // Track which model is being downloaded
    mpDownloadStatusLabel->setText(tr("Downloading %1...").arg(model.name));
    mpDownloadProgressBar->setValue(0);
    mpDownloadDetailsLabel->setText(tr("Connecting to server..."));

    // Create download directory
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    mDownloadedFilePath = downloadDir + QDir::separator() + model.identifier + qsl(".zip");

    // Open file for streaming download to disk (avoids loading entire payload into memory)
    mDownloadFile.setFileName(mDownloadedFilePath);
    if (!mDownloadFile.open(QIODevice::WriteOnly)) {
        mpDownloadStatusLabel->setText(tr("Failed to create download file!"));
        mpDownloadDetailsLabel->setText(tr("Could not open: %1").arg(mDownloadedFilePath));
        return;
    }

    QNetworkRequest request(QUrl(model.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    mpCurrentDownload = mpNetworkManager->get(request);
    connect(mpCurrentDownload, &QNetworkReply::downloadProgress, this, &dlgSpeechRecognitionSetup::slot_downloadProgress);
    connect(mpCurrentDownload, &QNetworkReply::readyRead, this, [this]() {
        if (mpCurrentDownload && mDownloadFile.isOpen()) {
            mDownloadFile.write(mpCurrentDownload->readAll());
        }
    });
    connect(mpCurrentDownload, &QNetworkReply::finished, this, &dlgSpeechRecognitionSetup::slot_downloadFinished);
}

void dlgSpeechRecognitionSetup::slot_downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        mpDownloadProgressBar->setValue(progress);
        mpDownloadDetailsLabel->setText(tr("Downloaded %1 of %2").arg(formatSize(bytesReceived)).arg(formatSize(bytesTotal)));
    } else {
        mpDownloadDetailsLabel->setText(tr("Downloaded %1").arg(formatSize(bytesReceived)));
    }
}

void dlgSpeechRecognitionSetup::slot_downloadFinished()
{
    // Close the download file (data was written incrementally via readyRead)
    if (mDownloadFile.isOpen()) {
        mDownloadFile.close();
    }

    if (!mpCurrentDownload) {
        return;
    }

    if (mpCurrentDownload->error() != QNetworkReply::NoError) {
        mpDownloadStatusLabel->setText(tr("Download failed!"));
        mpDownloadDetailsLabel->setText(tr("Error: %1").arg(mpCurrentDownload->errorString()));
        mpCompletionIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(48, 48));
        mpCompletionLabel->setText(tr("<b>Setup failed</b>"));
        mpCompletionDetailsLabel->setText(tr("The model could not be downloaded. Please check your internet connection and try again."));

        // Clean up partial download file
        QFile::remove(mDownloadedFilePath);

        mpCurrentDownload->deleteLater();
        mpCurrentDownload = nullptr;
        goToPage(CompletionPage);
        return;
    }

    mpCurrentDownload->deleteLater();
    mpCurrentDownload = nullptr;

    // Extract the model
    mpDownloadStatusLabel->setText(tr("Extracting model..."));
    mpDownloadProgressBar->setValue(100);

    // Use the models directory and the downloaded model identifier
    QString modelsDir = VoskRecognizer::modelsDirectoryPath();
    QString modelPath = modelsDir + QDir::separator() + mCurrentlyDownloadingModelId;
    QDir modelDir(modelPath);

    // Create models directory if needed
    QDir parentDir(modelsDir);
    if (!parentDir.exists()) {
        parentDir.mkpath(qsl("."));
    }

    if (extractModel(mDownloadedFilePath, modelsDir)) {
        mInstalledModelPath = modelPath;
        mSetupCompleted = true;

        // Save this model as the selected model
        VoskRecognizer::setSelectedModelPath(modelPath);

        // Clean up downloaded file
        QFile::remove(mDownloadedFilePath);

        mpCompletionIcon->setPixmap(style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(48, 48));
        mpCompletionLabel->setText(tr("<b>Setup completed successfully!</b>"));
        mpCompletionDetailsLabel->setText(tr("<p>The speech recognition model has been installed to:</p>"
                                             "<p><code>%1</code></p>"
                                             "<p>You can now use the microphone button in the command line to speak commands.</p>")
                                                  .arg(mInstalledModelPath));
    } else {
        mpCompletionIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(48, 48));
        mpCompletionLabel->setText(tr("<b>Extraction failed</b>"));
        mpCompletionDetailsLabel->setText(tr("<p>The model archive could not be extracted.</p>"
                                             "<p>You may need to manually extract the file:</p>"
                                             "<p><code>%1</code></p>"
                                             "<p>to:</p>"
                                             "<p><code>%2</code></p>")
                                                  .arg(mDownloadedFilePath, modelPath));
    }

    goToPage(CompletionPage);
}

bool dlgSpeechRecognitionSetup::extractModel(const QString& archivePath, const QString& destinationDir)
{
    QDir destDir(destinationDir);
    if (!destDir.exists()) {
        destDir.mkpath(qsl("."));
    }

    // Ensure destination ends with separator (mudlet::unzip concatenates paths directly)
    QString destination = destinationDir;
    if (!destination.endsWith(QDir::separator()) && !destination.endsWith(QLatin1Char('/'))) {
        destination += QDir::separator();
    }

    // Use mudlet's unzip function
    bool success = mudlet::unzip(archivePath, destination, destDir);
    if (!success) {
        return false;
    }

    // Check for and fix nested directory structure
    // Some Vosk model zips extract as model-name/model-name/... instead of model-name/...
    // We need to detect this and move contents up
    destDir.refresh();
    const QStringList subDirs = destDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subDirName : subDirs) {
        QString subDirPath = destDir.absoluteFilePath(subDirName);
        QDir subDir(subDirPath);

        // Check if this subdirectory contains another directory with the same name
        QString nestedPath = subDirPath + QDir::separator() + subDirName;
        QFileInfo nestedInfo(nestedPath);
        qDebug() << "dlgSpeechRecognitionSetup: Checking for nested dir:" << nestedPath << "exists:" << nestedInfo.exists() << "isDir:" << nestedInfo.isDir();

        if (nestedInfo.exists() && nestedInfo.isDir()) {
            QDir nestedDir(nestedPath);
            qDebug() << "dlgSpeechRecognitionSetup: Found nested directory, flattening:" << nestedPath;

            // Move all contents from nested directory up one level
            const QStringList nestedEntries = nestedDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
            bool moveSuccess = true;
            for (const QString& entry : nestedEntries) {
                QString sourcePath = nestedDir.absoluteFilePath(entry);
                QString destPath = subDir.absoluteFilePath(entry);

                QFileInfo sourceInfo(sourcePath);
                if (sourceInfo.isDir()) {
                    // For directories, rename the whole thing
                    if (!QDir().rename(sourcePath, destPath)) {
                        qWarning() << "dlgSpeechRecognitionSetup: Failed to move directory" << sourcePath << "to" << destPath;
                        moveSuccess = false;
                    }
                } else {
                    // For files, rename
                    if (!QFile::rename(sourcePath, destPath)) {
                        qWarning() << "dlgSpeechRecognitionSetup: Failed to move file" << sourcePath << "to" << destPath;
                        moveSuccess = false;
                    }
                }
            }

            // Remove the now-empty nested directory
            if (moveSuccess) {
                nestedDir.removeRecursively();
                qDebug() << "dlgSpeechRecognitionSetup: Fixed nested model directory structure in" << subDir.absolutePath();
            }
        }
    }

    return true;
}

void dlgSpeechRecognitionSetup::slot_openVoskWebsite()
{
    QDesktopServices::openUrl(QUrl(qsl("https://alphacephei.com/vosk/install")));
}

void dlgSpeechRecognitionSetup::slot_refreshLibraryStatus()
{
    // Reset the library load state to allow fresh detection
    // (needed after library installation)
    VoskRecognizer::resetLibraryLoadState();

    mVoskLibraryAvailable = checkVoskLibrary();
    updateNavigationButtons();
}

QString dlgSpeechRecognitionSetup::formatSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return tr("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return tr("%1 KB").arg(bytes / 1024);
    }
    if (bytes < 1024 * 1024 * 1024) {
        return tr("%1 MB").arg(bytes / (1024 * 1024));
    }
    return tr("%1 GB").arg(bytes / (1024 * 1024 * 1024));
}

dlgSpeechRecognitionSetup::LibraryInfo dlgSpeechRecognitionSetup::getLibraryInfoForCurrentPlatform() const
{
    // Vosk library releases from https://github.com/alphacep/vosk-api/releases
    // Note: macOS builds stopped at v0.3.42, Linux/Windows have v0.3.45

    LibraryInfo info;

#if defined(Q_OS_MACOS)
    // macOS - only universal build available (supports both ARM and Intel)
    // Last macOS release was v0.3.42
    info.platform = qsl("macos");
    info.url = qsl("https://github.com/alphacep/vosk-api/releases/download/v0.3.42/vosk-osx-0.3.42.zip");
    info.libraryName = qsl("libvosk.dylib");
    info.installPath = QCoreApplication::applicationDirPath() + qsl("/../Frameworks/");
    info.sizeBytes = 23 * 1024 * 1024; // ~23 MB
#elif defined(Q_OS_WIN)
#if defined(_WIN64) || defined(__x86_64__)
    info.platform = qsl("windows-x64");
    info.url = qsl("https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-win64-0.3.45.zip");
    info.libraryName = qsl("libvosk.dll");
    info.installPath = QCoreApplication::applicationDirPath() + QDir::separator();
    info.sizeBytes = 10 * 1024 * 1024; // ~10 MB
#else
    info.platform = qsl("windows-x86");
    info.url = qsl("https://github.com/alphacep/vosk-api/releases/download/v0.3.42/vosk-win32-0.3.42.zip");
    info.libraryName = qsl("libvosk.dll");
    info.installPath = QCoreApplication::applicationDirPath() + QDir::separator();
    info.sizeBytes = 10 * 1024 * 1024; // ~10 MB
#endif
#else
    // Linux
#if defined(__aarch64__)
    info.platform = qsl("linux-aarch64");
    info.url = qsl("https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-aarch64-0.3.45.zip");
    info.libraryName = qsl("libvosk.so");
    info.installPath = qsl("/usr/local/lib/");
    info.sizeBytes = 7 * 1024 * 1024; // ~7 MB
#else
    info.platform = qsl("linux-x64");
    info.url = qsl("https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-x86_64-0.3.45.zip");
    info.libraryName = qsl("libvosk.so");
    info.installPath = qsl("/usr/local/lib/");
    info.sizeBytes = 8 * 1024 * 1024; // ~8 MB
#endif
#endif

    return info;
}

void dlgSpeechRecognitionSetup::slot_downloadLibrary()
{
    if (mDownloadingLibrary || mpCurrentDownload) {
        return;
    }

    LibraryInfo libInfo = getLibraryInfoForCurrentPlatform();

    mDownloadingLibrary = true;
    mpDownloadLibraryButton->setEnabled(false);
    mpOpenVoskWebsiteButton->setEnabled(false);
    mpRefreshLibraryButton->setEnabled(false);
    mpBackButton->setEnabled(false);
    mpNextButton->setEnabled(false);

    mpLibraryDownloadStatusLabel->setText(tr("Downloading Vosk library for %1...").arg(libInfo.platform));
    mpLibraryDownloadStatusLabel->setVisible(true);
    mpLibraryDownloadProgressBar->setValue(0);
    mpLibraryDownloadProgressBar->setVisible(true);

    // Create download path
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    mLibraryDownloadPath = downloadDir + QDir::separator() + qsl("vosk-library.zip");

    QNetworkRequest request(QUrl(libInfo.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    mpCurrentDownload = mpNetworkManager->get(request);
    connect(mpCurrentDownload, &QNetworkReply::downloadProgress, this, &dlgSpeechRecognitionSetup::slot_libraryDownloadProgress);
    connect(mpCurrentDownload, &QNetworkReply::finished, this, &dlgSpeechRecognitionSetup::slot_libraryDownloadFinished);
}

void dlgSpeechRecognitionSetup::slot_libraryDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        mpLibraryDownloadProgressBar->setValue(progress);
        mpLibraryDownloadStatusLabel->setText(tr("Downloading: %1 of %2").arg(formatSize(bytesReceived)).arg(formatSize(bytesTotal)));
    } else {
        mpLibraryDownloadStatusLabel->setText(tr("Downloading: %1").arg(formatSize(bytesReceived)));
    }
}

void dlgSpeechRecognitionSetup::slot_libraryDownloadFinished()
{
    if (!mpCurrentDownload) {
        return;
    }

    mDownloadingLibrary = false;
    mpBackButton->setEnabled(true);
    mpOpenVoskWebsiteButton->setEnabled(true);
    mpRefreshLibraryButton->setEnabled(true);

    if (mpCurrentDownload->error() != QNetworkReply::NoError) {
        mpLibraryDownloadStatusLabel->setText(tr("<span style='color: red;'>Download failed: %1</span>").arg(mpCurrentDownload->errorString()));
        mpDownloadLibraryButton->setEnabled(true);
        mpCurrentDownload->deleteLater();
        mpCurrentDownload = nullptr;
        return;
    }

    // Save downloaded data to file
    QFile file(mLibraryDownloadPath);

    if (!file.open(QIODevice::WriteOnly)) {
        mpLibraryDownloadStatusLabel->setText(tr("<span style='color: red;'>Failed to save download</span>"));
        mpDownloadLibraryButton->setEnabled(true);
        mpCurrentDownload->deleteLater();
        mpCurrentDownload = nullptr;
        return;
    }

    file.write(mpCurrentDownload->readAll());
    file.close();

    mpCurrentDownload->deleteLater();
    mpCurrentDownload = nullptr;

    // Extract and install the library
    mpLibraryDownloadStatusLabel->setText(tr("Installing library..."));
    mpLibraryDownloadProgressBar->setValue(100);

    LibraryInfo libInfo = getLibraryInfoForCurrentPlatform();

    if (extractLibrary(mLibraryDownloadPath, libInfo.installPath)) {
        // Clean up downloaded file
        QFile::remove(mLibraryDownloadPath);

        mpLibraryDownloadStatusLabel->setText(tr("<span style='color: green;'>Library installed successfully!</span>"));
        mpLibraryDownloadProgressBar->setVisible(false);

        // Refresh the library status
        QTimer::singleShot(500, this, &dlgSpeechRecognitionSetup::slot_refreshLibraryStatus);
    } else {
        mpLibraryDownloadStatusLabel->setText(tr("<span style='color: red;'>Installation failed. You may need to manually copy the library.</span><br/>"
                                                 "Downloaded file: <code>%1</code><br/>"
                                                 "Install to: <code>%2</code>")
                                                      .arg(mLibraryDownloadPath, libInfo.installPath));
        mpDownloadLibraryButton->setEnabled(true);
    }
}

bool dlgSpeechRecognitionSetup::extractLibrary(const QString& archivePath, const QString& destinationDir)
{
    // Create destination directory if needed
    QDir destDir(destinationDir);

    if (!destDir.exists()) {
        if (!destDir.mkpath(qsl("."))) {
            qWarning() << "dlgSpeechRecognitionSetup: Failed to create directory:" << destinationDir;
            return false;
        }
    }

    // Check if we have write permission
    QFileInfo destInfo(destinationDir);

    if (!destInfo.isWritable()) {
        qWarning() << "dlgSpeechRecognitionSetup: No write permission to:" << destinationDir;

#if !defined(Q_OS_WIN)
        // On Unix systems, try to show a message about needing elevated permissions
        QMessageBox::warning(this,
                             tr("Permission Required"),
                             tr("Installing the library to %1 requires administrator privileges.\n\n"
                                "Please manually copy the library file from the downloaded archive:\n%2\n\n"
                                "Or run Mudlet with administrator privileges to install automatically.")
                                     .arg(destinationDir, archivePath));
#endif
        return false;
    }

    // Ensure destination ends with separator (mudlet::unzip concatenates paths directly)
    QString destination = destinationDir;

    if (!destination.endsWith(QDir::separator()) && !destination.endsWith(QLatin1Char('/'))) {
        destination += QDir::separator();
    }

    // Extract using mudlet's unzip function
    bool success = mudlet::unzip(archivePath, destination, destDir);

    if (!success) {
        qWarning() << "dlgSpeechRecognitionSetup: Failed to extract library archive";
        return false;
    }

    // The archive contains a folder structure, we need to find and move the library file
    LibraryInfo libInfo = getLibraryInfoForCurrentPlatform();

    // Search for a file in extracted contents (checks dest dir and subdirectories)
    auto findFileInExtracted = [](const QDir& dir, const QString& fileName) -> QString {
        // Check current directory
        if (dir.exists(fileName)) {
            return dir.absoluteFilePath(fileName);
        }

        // Check subdirectories (Vosk archives have a folder like "vosk-osx-0.3.45/")
        const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& subDir : subDirs) {
            QDir sub(dir.absoluteFilePath(subDir));

            if (sub.exists(fileName)) {
                return sub.absoluteFilePath(fileName);
            }
        }

        return QString();
    };

    // Helper to copy a file to destination, replacing if exists
    auto copyFileToDestination = [&destinationDir](const QString& srcPath, const QString& fileName) -> bool {
        if (srcPath.isEmpty()) {
            return false;
        }

        QString finalPath = QDir(destinationDir).filePath(fileName);

        if (srcPath == finalPath) {
            return true; // Already in place
        }

        // Remove existing file if present
        if (QFile::exists(finalPath)) {
            if (!QFile::remove(finalPath)) {
                QFile existingFile(finalPath);
                qWarning() << "dlgSpeechRecognitionSetup: Failed to remove existing file" << finalPath
                           << "for" << fileName << "-" << existingFile.errorString();
                return false;
            }
        }

        if (!QFile::copy(srcPath, finalPath)) {
            qWarning() << "dlgSpeechRecognitionSetup: Failed to copy" << fileName << "to final location";
            return false;
        }

        if (!QFile::remove(srcPath)) {
            QFile srcFile(srcPath);
            qWarning() << "dlgSpeechRecognitionSetup: Failed to remove source file" << srcPath
                       << "after copying" << fileName << "-" << srcFile.errorString();
        }

        return true;
    };

    // Find and install the main library
    QString extractedLibPath = findFileInExtracted(destDir, libInfo.libraryName);

    if (extractedLibPath.isEmpty()) {
        qWarning() << "dlgSpeechRecognitionSetup: Could not find" << libInfo.libraryName << "in extracted archive";
        return false;
    }

    if (!copyFileToDestination(extractedLibPath, libInfo.libraryName)) {
        return false;
    }

#if defined(Q_OS_WIN)
    // Windows: Vosk archive includes MinGW runtime DLLs that must also be installed
    // These are required dependencies for libvosk.dll to load
    QStringList windowsRuntimeDlls = {
            qsl("libstdc++-6.dll"),
            qsl("libwinpthread-1.dll"),
#if defined(_WIN64) || defined(__x86_64__)
            qsl("libgcc_s_seh-1.dll") // 64-bit Windows
#else
            qsl("libgcc_s_sjlj-1.dll") // 32-bit Windows
#endif
    };

    QStringList installedDlls;
    QStringList skippedDlls;
    QStringList failedDlls;

    for (const QString& dllName : windowsRuntimeDlls) {
        QString dllPath = findFileInExtracted(destDir, dllName);

        if (dllPath.isEmpty()) {
            skippedDlls << QStringLiteral("%1 (not found in archive)").arg(dllName);
        } else if (copyFileToDestination(dllPath, dllName)) {
            installedDlls << dllName;
        } else {
            failedDlls << QStringLiteral("%1 (copy failed)").arg(dllName);
        }
    }

    // Emit summary of runtime DLL installation results
    if (!installedDlls.isEmpty()) {
        qDebug().noquote() << "dlgSpeechRecognitionSetup: Installed runtime DLLs:" << installedDlls.join(qsl(", "));
    }

    if (!skippedDlls.isEmpty()) {
        qDebug().noquote() << "dlgSpeechRecognitionSetup: Skipped runtime DLLs:" << skippedDlls.join(qsl("; "));
    }

    if (!failedDlls.isEmpty()) {
        qWarning().noquote() << "dlgSpeechRecognitionSetup: Failed to install runtime DLLs:" << failedDlls.join(qsl("; "));
    }
    // Note: Not failing if runtime DLLs aren't found or fail to copy - they may already be on the system
    // or the user may have a newer Vosk build that statically links them
#endif

    // Clean up extracted directory contents (but not the library we just installed)
    const QStringList subDirs = destDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& subDir : subDirs) {
        if (subDir.startsWith(qsl("vosk-"))) {
            QDir sub(destDir.absoluteFilePath(subDir));
            sub.removeRecursively();
        }
    }

    qDebug() << "dlgSpeechRecognitionSetup: Library installed to:" << QDir(destinationDir).filePath(libInfo.libraryName);
    return true;
}
