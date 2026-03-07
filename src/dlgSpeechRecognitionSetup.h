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

#ifndef MUDLET_DLGSPEECHRECOGNITIONSETUP_H
#define MUDLET_DLGSPEECHRECOGNITIONSETUP_H

#include <QDialog>
#include <QFile>
#include <QPointer>

class QComboBox;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTextEdit;

// Wizard dialog for setting up speech recognition.
// Guides the user through:
// 1. Checking/installing the Vosk library
// 2. Selecting a language model
// 3. Downloading and installing the model

class dlgSpeechRecognitionSetup : public QDialog
{
    Q_OBJECT

public:
    explicit dlgSpeechRecognitionSetup(QWidget* parent = nullptr);
    ~dlgSpeechRecognitionSetup() override;

    // Returns true if setup completed successfully
    bool setupCompleted() const { return mSetupCompleted; }

    // Returns the path to the installed model
    QString installedModelPath() const { return mInstalledModelPath; }

private slots:
    void slot_nextPage();
    void slot_previousPage();
    void slot_downloadModel();
    void slot_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slot_downloadFinished();
    void slot_openVoskWebsite();
    void slot_refreshLibraryStatus();
    void slot_downloadLibrary();
    void slot_libraryDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slot_libraryDownloadFinished();

private:
    // Wizard pages
    enum Page {
        WelcomePage = 0,
        LibraryPage,
        ModelSelectionPage,
        DownloadPage,
        CompletionPage
    };

    // Model information
    struct ModelInfo {
        QString name;           // Display name
        QString identifier;     // Model identifier for download
        QString languageCode;   // ISO language code (e.g., "en", "de")
        QString url;            // Download URL
        qint64 sizeBytes;       // Approximate size in bytes
        bool isSmall;           // True for lightweight models
    };

    // Library information for auto-download
    struct LibraryInfo {
        QString platform;       // Platform identifier (e.g., "macos-arm64")
        QString url;            // Download URL
        QString libraryName;    // Name of library file in archive
        QString installPath;    // Where to install the library
        qint64 sizeBytes;       // Approximate size in bytes
    };

    void setupUi();
    void setupWelcomePage();
    void setupLibraryPage();
    void setupModelSelectionPage();
    void setupDownloadPage();
    void setupCompletionPage();

    void populateModelList();
    void updateNavigationButtons();
    void goToPage(Page page);
    bool checkVoskLibrary();
    QString getDefaultLanguageCode() const;
    bool extractModel(const QString& archivePath, const QString& destinationDir);
    QString formatSize(qint64 bytes) const;

    LibraryInfo getLibraryInfoForCurrentPlatform() const;
    void startLibraryDownload();
    bool extractLibrary(const QString& archivePath, const QString& destinationDir);

    // UI elements
    QStackedWidget* mpStackedWidget = nullptr;

    // Welcome page
    QLabel* mpWelcomeLabel = nullptr;

    // Library page
    QLabel* mpLibraryStatusIcon = nullptr;
    QLabel* mpLibraryStatusLabel = nullptr;
    QTextEdit* mpLibraryInstructions = nullptr;
    QPushButton* mpDownloadLibraryButton = nullptr;
    QPushButton* mpOpenVoskWebsiteButton = nullptr;
    QPushButton* mpRefreshLibraryButton = nullptr;
    QProgressBar* mpLibraryDownloadProgressBar = nullptr;
    QLabel* mpLibraryDownloadStatusLabel = nullptr;

    // Model selection page
    QComboBox* mpModelComboBox = nullptr;
    QLabel* mpModelDescriptionLabel = nullptr;
    QLabel* mpModelSizeLabel = nullptr;

    // Download page
    QLabel* mpDownloadStatusLabel = nullptr;
    QProgressBar* mpDownloadProgressBar = nullptr;
    QLabel* mpDownloadDetailsLabel = nullptr;

    // Completion page
    QLabel* mpCompletionIcon = nullptr;
    QLabel* mpCompletionLabel = nullptr;
    QLabel* mpCompletionDetailsLabel = nullptr;

    // Navigation buttons
    QPushButton* mpBackButton = nullptr;
    QPushButton* mpNextButton = nullptr;
    QPushButton* mpCancelButton = nullptr;

    // Network
    QNetworkAccessManager* mpNetworkManager = nullptr;
    QPointer<QNetworkReply> mpCurrentDownload;

    // State
    QList<ModelInfo> mAvailableModels;
    bool mVoskLibraryAvailable = false;
    bool mSetupCompleted = false;
    bool mDownloadingLibrary = false;
    QString mInstalledModelPath;
    QString mDownloadedFilePath;
    QFile mDownloadFile;  // File handle for streaming download to disk
    QString mLibraryDownloadPath;
    QString mCurrentlyDownloadingModelId;  // Model identifier being downloaded
};

#endif // MUDLET_DLGSPEECHRECOGNITIONSETUP_H
