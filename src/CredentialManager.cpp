/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "CredentialManager.h"
#include "SecureStringUtils.h"

#include "pre_guard.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDataStream>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include "../3rdparty/qtkeychain/keychain.h"
#else
#include <qt6keychain/keychain.h>
#endif
#include "post_guard.h"

// Forward declaration to avoid including mudlet.h
class mudlet;

// Helper function to check if mudlet is shutting down without including mudlet.h
static bool isMudletShuttingDown() {
    // For testing/minimal builds, we can simply return false
    // In the full application build, this can be replaced with actual mudlet checks
#ifndef CREDENTIALMANAGER_MINIMAL_BUILD
    // In the full build, we would include mudlet.h and use:
    // mudlet* instance = mudlet::self();
    // return instance ? instance->isGoingDown() : false;
    
    // For now, use a simple check - if QCoreApplication is closing, assume shutting down
    return QCoreApplication::closingDown();
#else
    // In minimal/test builds, assume not shutting down
    return false;
#endif
}

CredentialManager::CredentialManager(QObject* parent)
    : QObject(parent)
    , m_currentJob(nullptr)
    , m_timeoutTimer(nullptr)
{
}

CredentialManager::~CredentialManager()
{
    // During destruction, we should NOT call callbacks as they may reference
    // objects that are being destroyed. Instead, just clean up without callbacks.
    qDebug() << "CredentialManager: Destructor called, cleaning up without callbacks";
    
    // Clear callbacks before cleanup to prevent them from being called
    m_currentCallback = nullptr;
    m_currentRetrievalCallback = nullptr;
    m_currentAvailabilityCallback = nullptr;
    
    cleanupCurrentOperation();
}

void CredentialManager::setupTimeout()
{
    cleanupTimeout(); // Clean up any existing timer
    
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(OPERATION_TIMEOUT_MS);
    
    connect(m_timeoutTimer, &QTimer::timeout, this, &CredentialManager::handleTimeout);
    m_timeoutTimer->start();
}

void CredentialManager::cleanupTimeout()
{
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }
}

void CredentialManager::handleTimeout()
{
    qWarning() << "CredentialManager: Operation timed out";
    
    // Call appropriate callback with timeout error
    if (m_currentCallback) {
        m_currentCallback(false, "Operation timed out");
    } else if (m_currentRetrievalCallback) {
        m_currentRetrievalCallback(false, QString(), "Operation timed out");
    } else if (m_currentAvailabilityCallback) {
        m_currentAvailabilityCallback(false, "Operation timed out");
    }
    
    cleanupCurrentOperation();
}

void CredentialManager::cleanupCurrentOperation()
{
    cleanupTimeout();
    
    if (m_currentJob) {
        // Disconnect all signals to prevent callbacks after cleanup
        m_currentJob->disconnect();
        m_currentJob->deleteLater();
        m_currentJob = nullptr;
    }
    
    // Clear callbacks
    m_currentCallback = nullptr;
    m_currentRetrievalCallback = nullptr;
    m_currentAvailabilityCallback = nullptr;
}

// Additional safety method to check if we should proceed with keychain operations
// This prevents crashes when keychain operations complete during application shutdown
bool CredentialManager::isOperationValid() const
{
    // Check if we're in a valid state to handle callbacks
    // Multiple checks to catch different shutdown scenarios
    if (QCoreApplication::closingDown()) {
        qDebug() << "CredentialManager: Operation invalid - application closing down";
        return false;
    }
    
    // Check if mudlet is shutting down
    if (isMudletShuttingDown()) {
        qDebug() << "CredentialManager: Operation invalid - mudlet shutting down";
        return false;
    }
    
    // Check if our parent object is still valid (prevents crashes during widget destruction)
    if (parent() && parent()->property("__destroying").toBool()) {
        qDebug() << "CredentialManager: Operation invalid - parent object being destroyed";
        return false;
    }
    
    // Check if we have valid callbacks
    bool hasCallbacks = (m_currentCallback || m_currentRetrievalCallback || m_currentAvailabilityCallback);
    if (!hasCallbacks) {
        qDebug() << "CredentialManager: Operation invalid - no callbacks set";
    }
    
    return hasCallbacks;
}

bool CredentialManager::isPortableModeActive() const
{
    // Detect portable mode by checking for portable.txt markers
    // This uses the same logic as mudlet::setupConfig()
    
    QString confDirDefault = QString("%1/.config/mudlet").arg(QDir::homePath());
    
    // Find executable directory (same logic as findExecutableDir in mudlet.cpp)
    QString execDir;
    QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();
    if (systemEnvironment.contains("APPIMAGE")) {
        QString appimgPath = systemEnvironment.value("APPIMAGE", QString());
        execDir = QFileInfo(appimgPath).dir().path();
    } else {
        execDir = QCoreApplication::applicationDirPath();
    }
    
    QString markerExecDir = QString("%1/portable.txt").arg(execDir);
    QString markerHomeDir = QString("%1/portable.txt").arg(confDirDefault);
    
    // Check if either portable.txt marker exists
    return QFileInfo(markerExecDir).isFile() || QFileInfo(markerHomeDir).isFile();
}

bool CredentialManager::shouldUseKeychain(const QString& profileName) const
{
    Q_UNUSED(profileName)
    
    // If portable mode is active, prefer SecureStringUtils for portability
    if (isPortableModeActive()) {
        qDebug() << "CredentialManager: Using SecureStringUtils due to portable mode";
        return false;
    }
    
    // If in test environment, use SecureStringUtils to avoid keychain access
    if (SecureStringUtils::isTestEnvironment()) {
        qDebug() << "CredentialManager: Using SecureStringUtils due to test environment";
        return false;
    }
    
    // Otherwise, prefer keychain for better security
    qDebug() << "CredentialManager: Using keychain for better security";
    return true;
}

void CredentialManager::storePassword(const QString& profileName, const QString& key, 
                                     const QString& password, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, "Profile name and key cannot be empty");
        }
        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting storePassword operation during shutdown";
        if (callback) {
            callback(false, "Application is shutting down");
        }
        return;
    }
    
    if (shouldUseKeychain(profileName)) {
        // Use keychain storage
        QString service = generateServiceName(profileName, key);
        storeCredential(service, key, password, callback);
    } else {
        // Use SecureStringUtils for portable/test environments
        bool success = storeCredentialToFile(profileName, key, password);
        if (callback) {
            callback(success, success ? QString() : "Failed to store password with SecureStringUtils");
        }
    }
}

void CredentialManager::retrievePassword(const QString& profileName, const QString& key, 
                                        CredentialRetrievalCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, QString(), "Profile name and key cannot be empty");
        }
        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting retrievePassword operation during shutdown";
        if (callback) {
            callback(false, QString(), "Application is shutting down");
        }
        return;
    }
    
    if (shouldUseKeychain(profileName)) {
        // Use keychain storage - but also try SecureStringUtils as fallback
        QString service = generateServiceName(profileName, key);
        
        // First try keychain
        auto fallbackCallback = [this, profileName, key, callback](bool keychainSuccess, const QString& keychainPassword, const QString& keychainError) {
            if (keychainSuccess && !keychainPassword.isEmpty()) {
                // Keychain succeeded
                if (callback) {
                    callback(true, keychainPassword, QString());
                }
            } else {
                // Keychain failed, try legacy keychain format if this is a password request
                if (key == "password") {
                    qDebug() << "CredentialManager: Keychain failed, checking legacy format for profile:" << profileName;
                    checkLegacyKeychainFormat(profileName, [this, profileName, key, callback, keychainError](bool legacySuccess, const QString& legacyPassword) {
                        if (legacySuccess && !legacyPassword.isEmpty()) {
                            qDebug() << "CredentialManager: Found password in legacy keychain format, migrating to new format";
                            // Store in new format and remove from legacy
                            storePassword(profileName, key, legacyPassword, [callback, legacyPassword](bool migrationSuccess, const QString& migrationError) {
                                if (migrationSuccess) {
                                    qDebug() << "CredentialManager: Successfully migrated legacy password to new format";
                                } else {
                                    qWarning() << "CredentialManager: Failed to migrate legacy password:" << migrationError;
                                }
                                // Return the password regardless of migration result
                                if (callback) {
                                    callback(true, legacyPassword, QString());
                                }
                            });
                            // Clean up legacy entry asynchronously (don't wait for result)
                            deleteLegacyKeychainEntry(profileName);
                        } else {
                            // No legacy password found, try SecureStringUtils fallback
                            qDebug() << "CredentialManager: No legacy password found, trying SecureStringUtils fallback:" << keychainError;
                            QString fallbackPassword = retrieveCredentialFromFile(profileName, key);
                            if (!fallbackPassword.isEmpty()) {
                                qDebug() << "CredentialManager: Retrieved password from SecureStringUtils fallback";
                                if (callback) {
                                    callback(true, fallbackPassword, QString());
                                }
                            } else {
                                if (callback) {
                                    callback(false, QString(), QString("Keychain, legacy keychain, and SecureStringUtils all failed. Keychain error: %1").arg(keychainError));
                                }
                            }
                        }
                    });
                } else {
                    // Not a password request, try SecureStringUtils fallback directly
                    qDebug() << "CredentialManager: Keychain failed, trying SecureStringUtils fallback:" << keychainError;
                    QString fallbackPassword = retrieveCredentialFromFile(profileName, key);
                    if (!fallbackPassword.isEmpty()) {
                        qDebug() << "CredentialManager: Retrieved password from SecureStringUtils fallback";
                        if (callback) {
                            callback(true, fallbackPassword, QString());
                        }
                    } else {
                        if (callback) {
                            callback(false, QString(), QString("Both keychain and SecureStringUtils failed. Keychain error: %1").arg(keychainError));
                        }
                    }
                }
            }
        };
        
        retrieveCredential(service, key, fallbackCallback);
    } else {
        // Use SecureStringUtils directly
        QString password = retrieveCredentialFromFile(profileName, key);
        bool success = !password.isEmpty();
        if (callback) {
            callback(success, password, success ? QString() : "Failed to retrieve password with SecureStringUtils");
        }
    }
}

void CredentialManager::removePassword(const QString& profileName, const QString& key, 
                                      CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, "Profile name and key cannot be empty");
        }
        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting removePassword operation during shutdown";
        if (callback) {
            callback(false, "Application is shutting down");
        }
        return;
    }
    
    if (shouldUseKeychain(profileName)) {
        // Remove from both keychain and SecureStringUtils to ensure complete cleanup
        QString service = generateServiceName(profileName, key);
        
        auto combinedCallback = [this, profileName, key, callback](bool keychainSuccess, const QString& keychainError) {
            // Also remove from SecureStringUtils (ignore result as it might not exist there)
            bool fileSuccess = removeCredentialFromFile(profileName, key);
            
            if (callback) {
                if (keychainSuccess || fileSuccess) {
                    callback(true, QString());
                } else {
                    callback(false, QString("Failed to remove from keychain: %1").arg(keychainError));
                }
            }
        };
        
        removeCredential(service, key, combinedCallback);
    } else {
        // Use SecureStringUtils
        bool success = removeCredentialFromFile(profileName, key);
        if (callback) {
            callback(success, success ? QString() : "Failed to remove password with SecureStringUtils");
        }
    }
}

void CredentialManager::migratePassword(const QString& profileName, const QString& key, 
                                       const QString& plaintextPassword, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty() || plaintextPassword.isEmpty()) {
        if (callback) {
            callback(false, "Profile name, key, and password cannot be empty");
        }
        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting migratePassword operation during shutdown";
        if (callback) {
            callback(false, "Application is shutting down");
        }
        return;
    }
    
    qDebug() << "CredentialManager: Migrating plaintext password to encrypted storage for profile" << profileName << "key" << key;
    
    // Store the password using our hybrid approach
    storePassword(profileName, key, plaintextPassword, callback);
}

void CredentialManager::storeCredential(const QString& service, const QString& account, 
                                       const QString& password, CredentialCallback callback)
{
    if (service.isEmpty() || account.isEmpty()) {
        if (callback) {
            callback(false, "Service and account cannot be empty");
        }

        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting storeCredential operation during shutdown";
        if (callback) {
            callback(false, "Application is shutting down");
        }

        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    auto* writeJob = new QKeychain::WritePasswordJob(service, this);
    writeJob->setKey(account);
    
    // Store password directly in keychain (keychain handles encryption)
    writeJob->setTextData(password);
    writeJob->setAutoDelete(false);
    
    m_currentJob = writeJob;
    m_currentCallback = callback;
    
    // Set up timeout
    setupTimeout();
    
    // Connect signals with queued connection for safety
    connect(writeJob, &QKeychain::WritePasswordJob::finished, this, [this, writeJob, service, account, password]() {
        // Early exit if operation is no longer valid
        if (!isOperationValid()) {
            qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
            writeJob->deleteLater();
            return;
        }
        
        cleanupTimeout();
        
        bool success = (writeJob->error() == QKeychain::NoError);
        QString errorMessage = success ? QString() : writeJob->errorString();
        
        // If keychain failed, try file storage fallback
        if (!success) {
            qDebug() << "QtKeychain storage failed, attempting file fallback:" << errorMessage;
            
            // Use service as profile name and account as key for file storage
            bool fileSuccess = storeCredentialToFile(service, account, password);

            if (fileSuccess) {
                success = true;
                errorMessage = QString(); // Clear error message on successful fallback
                qDebug() << "File storage fallback succeeded";
            } else {
                errorMessage = QString("Both keychain and file storage failed. Keychain error: %1").arg(errorMessage);
            }
        }
        
        // Final validity check before calling callback
        if (m_currentCallback && isOperationValid()) {
            auto callback = m_currentCallback; // Copy callback to avoid use-after-free
            m_currentCallback = nullptr;
            m_currentJob = nullptr;
            
            callback(success, errorMessage);
        }
        
        writeJob->deleteLater();
    }, Qt::QueuedConnection); // Use queued connection for additional safety
    
    writeJob->start();
}

void CredentialManager::retrieveCredential(const QString& service, const QString& account, 
                                          CredentialRetrievalCallback callback)
{
    if (service.isEmpty() || account.isEmpty()) {
        if (callback) {
            callback(false, QString(), "Service and account cannot be empty");
        }
        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting retrieveCredential operation during shutdown";

        if (callback) {
            callback(false, QString(), "Application is shutting down");
        }

        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    auto* readJob = new QKeychain::ReadPasswordJob(service, this);
    readJob->setKey(account);
    readJob->setAutoDelete(false);
    
    m_currentJob = readJob;
    m_currentRetrievalCallback = callback;
    
    // Set up timeout
    setupTimeout();
    
    // Connect signals with queued connection for safety
    connect(readJob, &QKeychain::ReadPasswordJob::finished, this, [this, readJob, service, account]() {
        // Early exit if operation is no longer valid
        if (!isOperationValid()) {
            qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
            readJob->deleteLater();
            return;
        }
        
        cleanupTimeout();
        
        bool success = (readJob->error() == QKeychain::NoError);
        QString password;
        QString errorMessage;
        
        if (success) {
            // Get password directly from keychain (keychain handles decryption)
            password = readJob->textData();
            // No additional decryption needed for keychain passwords
        } else {
            // Keychain failed, try file storage fallback
            qDebug() << "QtKeychain retrieval failed, attempting file fallback:" << readJob->errorString();
            
            password = retrieveCredentialFromFile(service, account);

            if (!password.isNull()) {
                success = true;
                errorMessage = QString(); // Clear error message on successful fallback
                qDebug() << "File storage fallback succeeded";
            } else {
                errorMessage = QString("Both keychain and file storage failed. Keychain error: %1").arg(readJob->errorString());
            }
        }
        
        // Final validity check before calling callback
        if (m_currentRetrievalCallback && isOperationValid()) {
            auto callback = m_currentRetrievalCallback; // Copy callback to avoid use-after-free
            m_currentRetrievalCallback = nullptr;
            m_currentJob = nullptr;
            
            callback(success, password, errorMessage);
        }
        
        readJob->deleteLater();
    }, Qt::QueuedConnection); // Use queued connection for additional safety
    
    readJob->start();
}

void CredentialManager::removeCredential(const QString& service, const QString& account, 
                                        CredentialCallback callback)
{
    if (service.isEmpty() || account.isEmpty()) {
        if (callback) {
            callback(false, "Service and account cannot be empty");
        }

        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting removeCredential operation during shutdown";
        if (callback) {
            callback(false, "Application is shutting down");
        }

        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    auto* deleteJob = new QKeychain::DeletePasswordJob(service, this);
    deleteJob->setKey(account);
    deleteJob->setAutoDelete(false);
    
    m_currentJob = deleteJob;
    m_currentCallback = callback;
    
    // Set up timeout
    setupTimeout();
    
    // Connect signals with queued connection for safety
    connect(deleteJob, &QKeychain::DeletePasswordJob::finished, this, [this, deleteJob, service, account]() {
        // Early exit if operation is no longer valid
        if (!isOperationValid()) {
            qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
            deleteJob->deleteLater();
            return;
        }
        
        cleanupTimeout();
        
        bool keychainSuccess = (deleteJob->error() == QKeychain::NoError || 
                               deleteJob->error() == QKeychain::EntryNotFound);
        
        // Always try to remove from file storage as well (for cleanup)
        bool fileSuccess = removeCredentialFromFile(service, account);
        
        // Consider success if either method succeeded
        bool success = keychainSuccess || fileSuccess;
        QString errorMessage;
        
        if (!success) {
            errorMessage = QString("Failed to remove from both keychain and file storage. Keychain error: %1").arg(deleteJob->errorString());
        } else if (!keychainSuccess) {
            qDebug() << "Keychain removal failed but file removal succeeded:" << deleteJob->errorString();
        }
        
        // Final validity check before calling callback
        if (m_currentCallback && isOperationValid()) {
            auto callback = m_currentCallback; // Copy callback to avoid use-after-free
            m_currentCallback = nullptr;
            m_currentJob = nullptr;
            
            callback(success, errorMessage);
        }
        
        deleteJob->deleteLater();
    }, Qt::QueuedConnection); // Use queued connection for additional safety
    
    deleteJob->start();
}

void CredentialManager::isKeychainAvailable(AvailabilityCallback callback)
{
    if (!callback) {
        return;
    }

    // Check if we're in test environment
    if (SecureStringUtils::isTestEnvironment()) {
        callback(false, "Keychain disabled in test environment");
        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown() || isMudletShuttingDown()) {
        qWarning() << "CredentialManager: Rejecting isKeychainAvailable operation during shutdown";
        callback(false, "Application is shutting down");
        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    // Test keychain availability by trying to read a non-existent key
    auto* testJob = new QKeychain::ReadPasswordJob("MudletKeychainTest", this);
    testJob->setKey("availability_test");
    testJob->setAutoDelete(false);
    
    m_currentJob = testJob;
    m_currentAvailabilityCallback = callback;
    
    // Set up timeout
    setupTimeout();
    
    // Connect signals with queued connection for safety
    connect(testJob, &QKeychain::ReadPasswordJob::finished, this, [this, testJob]() {
        // Early exit if operation is no longer valid
        if (!isOperationValid()) {
            qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
            testJob->deleteLater();
            return;
        }
        
        cleanupTimeout();
        
        bool available = true;
        QString message = "Keychain is available";
        
        // Check for specific errors that indicate keychain is not available
        if (testJob->error() == QKeychain::AccessDenied ||
            testJob->error() == QKeychain::OtherError) {
            available = false;
            message = QString("Keychain not available: %1").arg(testJob->errorString());
        }
        
        // Final validity check before calling callback
        if (m_currentAvailabilityCallback && isOperationValid()) {
            auto callback = m_currentAvailabilityCallback; // Copy callback to avoid use-after-free
            m_currentAvailabilityCallback = nullptr;
            m_currentJob = nullptr;
            
            callback(available, message);
        }
        
        testJob->deleteLater();
    }, Qt::QueuedConnection); // Use queued connection for additional safety
    
    testJob->start();
}

// ============================================================================
// STATIC API (Synchronous file storage - for portable mode and backwards compatibility)
// 
// NOTE: This API uses encrypted file storage and is suitable for:
//   - Portable mode deployments where system keychain is not desired
//   - Backwards compatibility with existing synchronous code
// For QtKeychain integration with secure system keychain storage, please use
// the async API methods (storeCredential, retrieveCredential, removeCredential
// with callbacks) which provide:
//   - Primary storage in system keychain (macOS Keychain, Windows Credential Store, Linux Secret Service)
//   - Automatic fallback to encrypted file storage when keychain unavailable
//   - Better security and user experience
// ============================================================================

bool CredentialManager::storeCredential(const QString& profileName, const QString& key, const QString& credential)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }

    // Validate key name to prevent directory traversal and other security issues
    if (!isValidKeyName(key)) {
        return false;
    }

    // Log migration recommendation (only once per session to avoid spam)
    static bool migrationWarningLogged = false;

    if (!migrationWarningLogged) {
        qDebug() << "CredentialManager: Static API currently uses file storage only.";
        qDebug() << "CredentialManager: For QtKeychain integration, migrate to async API: storeCredential(service, account, password, callback)";
        migrationWarningLogged = true;
    }

    // Static API uses encrypted file storage for synchronous operations and portable mode
    // NOTE: For QtKeychain integration, use the async API methods instead.
    //       Main UI components (dlgConnectionProfiles) have been migrated to async API.
    return storeCredentialToFile(profileName, key, credential);
}

QString CredentialManager::retrieveCredential(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return QString();
    }

    // Validate key name to prevent directory traversal and other security issues
    if (!isValidKeyName(key)) {
        return QString();
    }

    // Static API uses encrypted file storage for synchronous operation
    // NOTE: For QtKeychain integration, use the async API methods instead.
    //       Main UI components (dlgConnectionProfiles) have been migrated to async API.
    return retrieveCredentialFromFile(profileName, key);
}

bool CredentialManager::removeCredential(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }

    // Validate key name to prevent directory traversal and other security issues
    if (!isValidKeyName(key)) {
        return false;
    }

    // Static API uses encrypted file storage for synchronous operation
    // NOTE: For QtKeychain integration, use the async API methods instead.
    //       Main UI components (dlgConnectionProfiles) have been migrated to async API.
    return removeCredentialFromFile(profileName, key);
}

bool CredentialManager::storeCredentialToFile(const QString& profileName, const QString& key, const QString& credential)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Check if path generation failed due to validation
    if (filePath.isEmpty()) {
        qWarning() << "CredentialManager: Failed to generate valid file path for storing encrypted credential";
        return false;
    }
    
    // Validate credential input - empty is allowed (represents "no password")
    // Only reject null QString which indicates a programming error
    if (credential.isNull()) {
        qWarning() << "CredentialManager: Null credential provided for storage";
        return false;
    }
    
    // Create directory structure if it doesn't exist
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();

    if (!dir.mkpath(dir.absolutePath())) {
        qWarning() << "CredentialManager: Failed to create directory structure for" << filePath;
        return false;
    }
    
    // Encrypt credential using profile-specific key (empty credentials are allowed)
    QString encrypted = SecureStringUtils::encryptStringForProfile(credential, profileName);

    if (encrypted.isEmpty() && !credential.isEmpty()) {
        qWarning() << "CredentialManager: Failed to encrypt credential for profile" << profileName;
        return false;
    }
    
    // Write to file
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "CredentialManager: Failed to open file for writing:" << filePath << "Error:" << file.errorString();
        return false;
    }
    
    qint64 bytesWritten = file.write(encrypted.toUtf8());
    file.close();
    
    // Verify write operation succeeded
    if (bytesWritten == -1) {
        qWarning() << "CredentialManager: Failed to write encrypted credential to file:" << filePath;
        return false;
    }
    
    return true;
}

QString CredentialManager::retrieveCredentialFromFile(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Check if path generation failed due to validation
    if (filePath.isEmpty()) {
        qWarning() << "CredentialManager: Failed to generate valid file path for retrieving encrypted credential";
        return QString();
    }
    
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Only log warning if file should exist (not for first-time access)
        if (file.exists()) {
            qWarning() << "CredentialManager: Failed to open existing file for reading:" << filePath << "Error:" << file.errorString();
        }

        return QString();
    }
    
    QString encrypted = QString::fromUtf8(file.readAll());
    file.close();
    
    if (encrypted.isEmpty()) {
        qWarning() << "CredentialManager: Retrieved empty encrypted data from file:" << filePath;
        return QString();
    }
    
    // Decrypt credential using profile-specific key
    QString decrypted = SecureStringUtils::decryptStringForProfile(encrypted, profileName);
    
    if (decrypted.isEmpty()) {
        qWarning() << "CredentialManager: Failed to decrypt credential for profile" << profileName;
    }
    
    return decrypted;
}

bool CredentialManager::removeCredentialFromFile(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Check if path generation failed due to validation
    if (filePath.isEmpty()) {
        qWarning() << "CredentialManager: Failed to generate valid file path for removing encrypted credential";
        return false;
    }
    
    // Check if file exists before attempting removal
    if (!QFile::exists(filePath)) {
        // Not an error - credential may not have been stored or already removed
        return true;
    }
    
    bool removed = QFile::remove(filePath);

    if (!removed) {
        qWarning() << "CredentialManager: Failed to remove encrypted credential file:" << filePath;
    }
    
    return removed;
}

QString CredentialManager::generateServiceName(const QString& profileName, const QString& key)
{
    // Sanitize inputs to prevent keychain service name conflicts
    auto sanitizeForService = [](const QString& input) -> QString {
        QString sanitized = input;
        // Replace invalid characters with underscores
        sanitized.replace(QRegularExpression(R"([^\w\-\.])"), "_");
        // Limit length to prevent overly long service names
        if (sanitized.length() > 50) {
            sanitized = sanitized.left(50);
        }

        return sanitized;
    };
    
    QString sanitizedProfile = sanitizeForService(profileName);
    QString sanitizedKey = sanitizeForService(key);
    
    return QString("Mudlet-%1-%2").arg(sanitizedProfile, sanitizedKey);
}

QString CredentialManager::generateFilePath(const QString& profileName, const QString& key)
{
    // Validate and sanitize file path components
    if (profileName.isEmpty() || key.isEmpty()) {
        qWarning() << "CredentialManager: Empty profile name or key provided for file path";
        return QString();
    }
    
    // Check for invalid characters that could cause path traversal or filesystem issues
    QRegularExpression pathTraversalPattern(R"(\.\.|[<>:"|?*\x00-\x1f])");

    if (profileName.contains(pathTraversalPattern) || key.contains(pathTraversalPattern)) {
        qWarning() << "CredentialManager: Invalid characters detected in path components";
        return QString();
    }
    
    auto sanitizeForPath = [](const QString& input) -> QString {
        QString sanitized = input;
        // Replace filesystem-unsafe characters with underscores
        sanitized.replace(QRegularExpression(R"([/\\:*?"<>|])"), "_");
        // Limit length to prevent filesystem issues
        if (sanitized.length() > 50) {
            sanitized = sanitized.left(50);
        }

        return sanitized;
    };
    
    QString sanitizedProfile = sanitizeForPath(profileName);
    QString sanitizedKey = sanitizeForPath(key);
    
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QString("%1/profiles/%2/passwords/%3").arg(configPath, sanitizedProfile, sanitizedKey);
}

bool CredentialManager::isValidKeyName(const QString& key)
{
    // Validate key name to prevent directory traversal and other security issues
    if (key.isEmpty() || key.length() > 100) {
        return false;
    }
    
    // Disallow dangerous characters and patterns
    QRegularExpression dangerousPattern(R"(\.\.|[<>:"|?*\x00-\x1f/\\])");
    return !key.contains(dangerousPattern);
}

void CredentialManager::checkLegacyKeychainFormat(const QString& profileName, 
                                                 std::function<void(bool, const QString&)> callback)
{
    if (profileName.isEmpty() || !callback) {
        if (callback) {
            callback(false, QString());
        }
        return;
    }
    
    // Legacy format used service="Mudlet profile" and key=profileName
    const QString legacyService = "Mudlet profile";
    
    auto* readJob = new QKeychain::ReadPasswordJob(legacyService, this);
    readJob->setKey(profileName);
    readJob->setAutoDelete(false);
    
    connect(readJob, &QKeychain::ReadPasswordJob::finished, this, [readJob, callback]() {
        bool success = (readJob->error() == QKeychain::NoError);
        QString password = success ? readJob->textData() : QString();
        
        if (success) {
            qDebug() << "CredentialManager: Found password in legacy keychain format";
        } else {
            qDebug() << "CredentialManager: No password found in legacy keychain format:" << readJob->errorString();
        }
        
        callback(success, password);
        readJob->deleteLater();
    });
    
    readJob->start();
}

void CredentialManager::deleteLegacyKeychainEntry(const QString& profileName)
{
    if (profileName.isEmpty()) {
        return;
    }
    
    // Legacy format used service="Mudlet profile" and key=profileName
    const QString legacyService = "Mudlet profile";
    
    auto* deleteJob = new QKeychain::DeletePasswordJob(legacyService, this);
    deleteJob->setKey(profileName);
    deleteJob->setAutoDelete(false);
    
    connect(deleteJob, &QKeychain::DeletePasswordJob::finished, this, [deleteJob, profileName]() {
        if (deleteJob->error() == QKeychain::NoError) {
            qDebug() << "CredentialManager: Successfully deleted legacy keychain entry for profile:" << profileName;
        } else {
            qDebug() << "CredentialManager: Failed to delete legacy keychain entry for profile:" << profileName << "Error:" << deleteJob->errorString();
        }
        deleteJob->deleteLater();
    });
    
    deleteJob->start();
}
