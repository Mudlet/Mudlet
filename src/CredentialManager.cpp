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
#include "utils.h"

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
#include <QVersionNumber>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include "../3rdparty/qtkeychain/keychain.h"
#else
#include <qt6keychain/keychain.h>
#endif
#include "post_guard.h"

// Forward declaration to avoid including mudlet.h
class mudlet;

CredentialManager::CredentialManager(QObject* parent)
    : QObject(parent)
    , mCurrentJob(nullptr)
    , mTimeoutTimer(nullptr)
{
}

CredentialManager::~CredentialManager()
{
    // Set destruction flag to prevent any new operations or callbacks
    mShuttingDown = true;
    
    // During destruction, we should NOT call callbacks as they may reference
    // objects that are being destroyed. Instead, just clean up without callbacks.
    
    // Clear callbacks before cleanup to prevent them from being called
    mCurrentCallback = nullptr;
    mCurrentRetrievalCallback = nullptr;
    mCurrentAvailabilityCallback = nullptr;
    
    // Clean up operations - this is safe even during application shutdown
    cleanupCurrentOperation();
}

void CredentialManager::setupTimeout()
{
    cleanupTimeout(); // Clean up any existing timer
    
    mTimeoutTimer = new QTimer(this);
    mTimeoutTimer->setSingleShot(true);
    mTimeoutTimer->setInterval(OPERATION_TIMEOUT_MS);
    
    connect(mTimeoutTimer, &QTimer::timeout, this, &CredentialManager::handleTimeout);
    mTimeoutTimer->start();
}

void CredentialManager::cleanupTimeout()
{
    if (mTimeoutTimer) {
        // Safely stop and disconnect the timer
        mTimeoutTimer->stop();
        
        // If we're destroying or shutting down, avoid disconnect calls that might crash
        if (!mShuttingDown && !QCoreApplication::closingDown()) {
            // Safe to disconnect during normal operation
            mTimeoutTimer->disconnect();
        }
        
        mTimeoutTimer->deleteLater();
        mTimeoutTimer = nullptr;
    }
}

void CredentialManager::handleTimeout()
{
    qWarning() << "CredentialManager: Operation timed out";
    
    // Call appropriate callback with timeout error
    if (mCurrentCallback) {
        mCurrentCallback(false, qsl("Operation timed out"));
    } else if (mCurrentRetrievalCallback) {
        mCurrentRetrievalCallback(false, QString(), qsl("Operation timed out"));
    } else if (mCurrentAvailabilityCallback) {
        mCurrentAvailabilityCallback(false, qsl("Operation timed out"));
    }
    
    cleanupCurrentOperation();
}

void CredentialManager::cleanupCurrentOperation()
{
    cleanupTimeout();

    if (mCurrentJob) {
        // If we're destroying or shutting down, avoid disconnect calls that might crash
        if (!mShuttingDown && !QCoreApplication::closingDown()) {
            // Safe to disconnect during normal operation
            mCurrentJob->disconnect();
        }

        mCurrentJob->deleteLater();
        mCurrentJob = nullptr;
    }
    
    // Clear callbacks
    mCurrentCallback = nullptr;
    mCurrentRetrievalCallback = nullptr;
    mCurrentAvailabilityCallback = nullptr;
}

// Safety method to check if we should proceed with keychain operations
bool CredentialManager::isOperationValid() const
{
    // Check if we're being destroyed
    if (mShuttingDown) {
        qDebug() << "CredentialManager: Operation invalid - object being destroyed";
        return false;
    }
    
    // Check if application is shutting down
    if (QCoreApplication::closingDown()) {
        qDebug() << "CredentialManager: Operation invalid - application shutting down";
        return false;
    }
    
    // Check if we have valid callbacks
    bool hasCallbacks = (mCurrentCallback || mCurrentRetrievalCallback || mCurrentAvailabilityCallback);

    if (!hasCallbacks) {
        qDebug() << "CredentialManager: Operation invalid - no callbacks set";
    }
    
    return hasCallbacks;
}

bool CredentialManager::isPortableModeActive() const
{
    // Ideally, this should be supplied by mudlet instance rather than
    // duplicating logic here. However, including mudlet.h creates circular dependencies.
    // Consider refactoring to get portable mode status from a shared utility or
    // through dependency injection.
    
    // Detect portable mode by checking for portable.txt markers
    // This uses the same logic as mudlet::setupConfig()
    
    QString confDirDefault = qsl("%1/.config/mudlet").arg(QDir::homePath());
    
    // Find executable directory (same logic as findExecutableDir in mudlet.cpp)
    QString execDir;
    QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();

    if (systemEnvironment.contains("APPIMAGE")) {
        QString appimgPath = systemEnvironment.value("APPIMAGE", QString());
        execDir = QFileInfo(appimgPath).dir().path();
    } else {
        execDir = QCoreApplication::applicationDirPath();
    }
    
    QString markerExecDir = qsl("%1/portable.txt").arg(execDir);
    QString markerHomeDir = qsl("%1/portable.txt").arg(confDirDefault);
    
    // Check if either portable.txt marker exists
    return QFileInfo(markerExecDir).isFile() || QFileInfo(markerHomeDir).isFile();
}

bool CredentialManager::shouldUseKeychain(const QString& profileName) const
{
    Q_UNUSED(profileName)
    
    // If portable mode is active, prefer SecureStringUtils for portability
    if (isPortableModeActive()) {
        qDebug() << "CredentialManager: Using encrypted storage (portable mode)";
        return false;
    }
    
    // If in test environment, use SecureStringUtils to avoid keychain access
    if (SecureStringUtils::isTestEnvironment()) {
        qDebug() << "CredentialManager: Using encrypted storage (test mode)";
        return false;
    }
    
    // Otherwise, prefer keychain for better security
    qDebug() << "CredentialManager: Using keychain storage";
    return true;
}

void CredentialManager::storePassword(const QString& profileName, const QString& key, 
                                     const QString& password, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, qsl("Profile name and key cannot be empty"));
        }

        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting storePassword operation during shutdown";

        if (callback) {
            callback(false, qsl("Application is shutting down"));
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
            callback(success, success ? QString() : qsl("Failed to store password with SecureStringUtils"));
        }
    }
}

void CredentialManager::retrievePassword(const QString& profileName, const QString& key, 
                                        CredentialRetrievalCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, QString(), qsl("Profile name and key cannot be empty"));
        }

        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting retrievePassword operation during shutdown";

        if (callback) {
            callback(false, QString(), qsl("Application is shutting down"));
        }

        return;
    }
    
    if (shouldUseKeychain(profileName)) {
        // Use keychain storage - but also try SecureStringUtils as fallback
        QString service = generateServiceName(profileName, key);
        
        // First try keychain
        auto fallbackCallback = [this, profileName, key, callback](bool keychainSuccess, const QString& keychainPassword, const QString& keychainError) {
            qDebug() << "CredentialManager: Keychain result for profile" << profileName << "- success:" << keychainSuccess << "password empty:" << keychainPassword.isEmpty();
            
            if (keychainSuccess && !keychainPassword.isEmpty()) {
                // Keychain succeeded
                if (callback) {
                    callback(true, keychainPassword, QString());
                }
            } else {
                // Keychain failed, try legacy keychain format if this is a password request
                if (key == "password" || key == "character") {
                    qDebug() << "CredentialManager: Checking for legacy keychain format for profile" << profileName;
                    checkLegacyKeychainFormat(profileName, [this, profileName, key, callback, keychainError](bool legacySuccess, const QString& legacyPassword) {
                        if (legacySuccess && !legacyPassword.isEmpty()) {
                            qDebug() << "CredentialManager: Migrating legacy password for profile" << profileName;
                            // Store in new format and remove from legacy
                            storePassword(profileName, key, legacyPassword, [callback, legacyPassword](bool migrationSuccess, const QString& migrationError) {
                                if (migrationSuccess) {
                                    qDebug() << "CredentialManager: Legacy migration successful";
                                } else {
                                    qWarning() << "CredentialManager: Legacy migration failed:" << migrationError;
                                }
                                // Return the password regardless of migration result
                                if (callback) {
                                    callback(true, legacyPassword, QString());
                                }
                            });
                            
                            // Only clean up legacy entry if this version is >= 4.20.0
                            // This prevents breaking compatibility with older Mudlet versions
#ifdef APP_VERSION
                            const QString currentVersion = QString(APP_VERSION);
                            const QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
                            const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);
                            
                            if (appVersion >= secureStorageVersion) {
                                // Clean up legacy entry asynchronously (don't wait for result)
                                deleteLegacyKeychainEntry(profileName);
                                qDebug() << "CredentialManager: Legacy cleanup enabled for version" << currentVersion;
                            } else {
                                qDebug() << "CredentialManager: Legacy cleanup skipped for version" << currentVersion;
                            }
#else
                            // In test environment, preserve legacy entries for safety
                            qDebug() << "CredentialManager: Legacy cleanup skipped (test mode)";
#endif
                        } else {
                            // No legacy password found, try encrypted file storage
                            qDebug() << "CredentialManager: No legacy password found, using encrypted file storage for profile" << profileName;
                            QString fallbackPassword = retrieveCredentialFromFile(profileName, key);

                            if (!fallbackPassword.isEmpty()) {
                                qDebug() << "CredentialManager: Password retrieved from encrypted file storage";

                                if (callback) {
                                    callback(true, fallbackPassword, QString());
                                }
                            } else {
                                if (callback) {
                                    callback(false, QString(), qsl("No stored credentials found for profile %1 (keychain, legacy, and encrypted file storage all empty)").arg(profileName));
                                }
                            }
                        }
                    });
                } else {
                    // Not a password request, use encrypted file storage directly
                    qDebug() << "CredentialManager: Using encrypted file storage for credential" << key << "for profile" << profileName;
                    QString fallbackPassword = retrieveCredentialFromFile(profileName, key);

                    if (!fallbackPassword.isEmpty()) {
                        qDebug() << "CredentialManager: Retrieved credential from encrypted file storage";

                        if (callback) {
                            callback(true, fallbackPassword, QString());
                        }
                    } else {
                        if (callback) {
                            callback(false, QString(), qsl("No stored credentials found for profile %1 (keychain and encrypted file storage both empty)").arg(profileName));
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
            callback(success, password, success ? QString() : qsl("Failed to retrieve password with SecureStringUtils"));
        }
    }
}

void CredentialManager::removePassword(const QString& profileName, const QString& key, 
                                      CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, qsl("Profile name and key cannot be empty"));
        }
        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting removePassword operation during shutdown";

        if (callback) {
            callback(false, qsl("Application is shutting down"));
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
                    callback(false, qsl("Failed to remove from keychain: %1").arg(keychainError));
                }
            }
        };
        
        removeCredential(service, key, combinedCallback);
    } else {
        // Use SecureStringUtils
        bool success = removeCredentialFromFile(profileName, key);

        if (callback) {
            callback(success, success ? QString() : qsl("Failed to remove password with SecureStringUtils"));
        }
    }
}

void CredentialManager::migratePassword(const QString& profileName, const QString& key, 
                                       const QString& plaintextPassword, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty() || plaintextPassword.isEmpty()) {
        if (callback) {
            callback(false, qsl("Profile name, key, and password cannot be empty"));
        }

        return;
    }
    
    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting migratePassword operation during shutdown";

        if (callback) {
            callback(false, qsl("Application is shutting down"));
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
            callback(false, qsl("Service and account cannot be empty"));
        }

        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting storeCredential operation during shutdown";

        if (callback) {
            callback(false, qsl("Application is shutting down"));
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
    
    mCurrentJob = writeJob;
    mCurrentCallback = callback;
    
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
            qDebug() << "CredentialManager: Keychain storage failed, using encrypted file storage:" << errorMessage;
            
            // Use service as profile name and account as key for file storage
            bool fileSuccess = storeCredentialToFile(service, account, password);

            if (fileSuccess) {
                success = true;
                errorMessage = QString(); // Clear error message on successful fallback
                qDebug() << "CredentialManager: Password stored to encrypted file storage";
            } else {
                errorMessage = qsl("Both keychain and encrypted file storage failed. Keychain error: %1").arg(errorMessage);
            }
        } else {
            qDebug() << "CredentialManager: Password stored to keychain service:" << service;
        }
        
        // Final validity check before calling callback
        if (mCurrentCallback && isOperationValid()) {
            auto callback = mCurrentCallback; // Copy callback to avoid use-after-free
            mCurrentCallback = nullptr;
            mCurrentJob = nullptr;
            
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
            callback(false, QString(), qsl("Service and account cannot be empty"));
        }

        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting retrieveCredential operation during shutdown";

        if (callback) {
            callback(false, QString(), qsl("Application is shutting down"));
        }

        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    auto* readJob = new QKeychain::ReadPasswordJob(service, this);
    readJob->setKey(account);
    readJob->setAutoDelete(false);
    
    mCurrentJob = readJob;
    mCurrentRetrievalCallback = callback;
    
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
            qDebug() << "CredentialManager: Retrieved password from keychain service:" << service;
        } else {
            // Keychain failed, try file storage fallback with specific error context
            QString errorContext;
            switch (readJob->error()) {
                case QKeychain::EntryNotFound:
                    errorContext = "No password stored in keychain";
                    break;
                case QKeychain::AccessDeniedByUser:
                    errorContext = "User denied keychain access";
                    break;
                case QKeychain::AccessDenied:
                    errorContext = "Keychain access denied by system";
                    break;
                case QKeychain::NoBackendAvailable:
                    errorContext = "No keychain service available";
                    break;
                case QKeychain::NotImplemented:
                    errorContext = "Keychain not supported on this platform";
                    break;
                default:
                    errorContext = qsl("Keychain error: %1").arg(readJob->errorString());
                    break;
            }
            qDebug() << "CredentialManager:" << errorContext << ", trying fallback storage";
            
            // Try legacy keychain format if this is a character password and service follows new format
            if (account == "character" && service.startsWith("Mudlet-") && service.endsWith("-character")) {
                // Extract profile name from service (format: "Mudlet-ProfileName-character")
                QString profileName = service.mid(7); // Remove "Mudlet-" prefix
                profileName.chop(10); // Remove "-character" suffix
                
                qDebug() << "CredentialManager: Checking for legacy keychain format for profile" << profileName;
                
                // Capture the error string now while readJob is still valid
                QString keychainError = readJob->errorString();
                
                // Check legacy keychain format asynchronously
                auto* legacyReadJob = new QKeychain::ReadPasswordJob("Mudlet profile", this);
                legacyReadJob->setKey(profileName);
                legacyReadJob->setAutoDelete(false);
                
                // Store the current callback and clear it to prevent double-calling
                auto originalCallback = mCurrentRetrievalCallback;
                mCurrentRetrievalCallback = nullptr;
                
                connect(legacyReadJob, &QKeychain::ReadPasswordJob::finished, this, [this, legacyReadJob, profileName, service, account, originalCallback, keychainError]() {
                    bool legacyFound = (legacyReadJob->error() == QKeychain::NoError);
                    QString legacyPassword = legacyFound ? legacyReadJob->textData() : QString();
                    
                    if (legacyFound && !legacyPassword.isEmpty()) {
                        qDebug() << "CredentialManager: Found legacy password for profile" << profileName;
                        
                        // Migrate to new format asynchronously
                        qDebug() << "CredentialManager: Migrating legacy password to new format for profile" << profileName;
                        
                        // Extract original profile name and key from service name for migration
                        QString originalProfileName = service.mid(7); // Remove "Mudlet-" prefix
                        originalProfileName.chop(10); // Remove "-character" suffix
                        
                        // Store in new format
                        auto* migrationManager = new CredentialManager();
                        migrationManager->storePassword(originalProfileName, account, legacyPassword, 
                            [migrationManager, profileName, originalCallback, legacyPassword](bool migrationSuccess, const QString& migrationError) {
                                if (migrationSuccess) {
                                    qDebug() << "CredentialManager: Legacy migration successful for profile" << profileName;
                                    
                                    // Clean up legacy entry if migration succeeded
#ifdef APP_VERSION
                                    const QString currentVersion = QString(APP_VERSION);
                                    const QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
                                    const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);
                                    
                                    if (appVersion >= secureStorageVersion) {
                                        // Clean up legacy entry asynchronously
                                        auto* cleanupManager = new CredentialManager();
                                        cleanupManager->deleteLegacyKeychainEntry(profileName);
                                        cleanupManager->deleteLater();
                                        qDebug() << "CredentialManager: Legacy cleanup initiated for version" << currentVersion;
                                    } else {
                                        qDebug() << "CredentialManager: Legacy cleanup skipped for version" << currentVersion;
                                    }
#else
                                    qDebug() << "CredentialManager: Legacy cleanup skipped (test mode)";
#endif
                                } else {
                                    qWarning() << "CredentialManager: Legacy migration failed for profile" << profileName << ":" << migrationError;
                                }
                                
                                // Return the legacy password regardless of migration result
                                if (originalCallback) {
                                    originalCallback(true, legacyPassword, QString());
                                }
                                
                                migrationManager->deleteLater();
                            });
                    } else {
                        qDebug() << "CredentialManager: No legacy password found for profile" << profileName;
                        
                        // No legacy password, try file storage
                        QString filePassword = retrieveCredentialFromFile(service, account);
                        bool fileSuccess = !filePassword.isNull();
                        QString finalError = fileSuccess ? QString() : qsl("Both keychain and encrypted file storage failed for credentials. Keychain: %1").arg(keychainError);
                        
                        if (fileSuccess) {
                            qDebug() << "CredentialManager: Retrieved password from encrypted file storage";
                        }
                        
                        if (originalCallback) {
                            originalCallback(fileSuccess, filePassword, finalError);
                        }
                    }
                    
                    legacyReadJob->deleteLater();
                });
                
                legacyReadJob->start();
                
                // Early return to avoid the file storage check below
                readJob->deleteLater();
                return;
            }
            
            // Not a character password or not new format, try file storage directly
            password = retrieveCredentialFromFile(service, account);

            if (!password.isNull()) {
                success = true;
                errorMessage = QString(); // Clear error message on successful fallback
                qDebug() << "CredentialManager: Retrieved password from encrypted file storage";
            } else {
                errorMessage = qsl("Both keychain and encrypted file storage failed for credentials. Keychain: %1").arg(readJob->errorString());
            }
        }
        
        // Final validity check before calling callback
        if (mCurrentRetrievalCallback && isOperationValid()) {
            auto callback = mCurrentRetrievalCallback; // Copy callback to avoid use-after-free
            mCurrentRetrievalCallback = nullptr;
            mCurrentJob = nullptr;
            
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
    if (QCoreApplication::closingDown()) {
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
    
    mCurrentJob = deleteJob;
    mCurrentCallback = callback;
    
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
            errorMessage = qsl("Failed to remove from both keychain and file storage. Keychain error: %1").arg(deleteJob->errorString());
        } else if (!keychainSuccess) {
            qDebug() << "Keychain removal failed but file removal succeeded:" << deleteJob->errorString();
        }
        
        // Final validity check before calling callback
        if (mCurrentCallback && isOperationValid()) {
            auto callback = mCurrentCallback; // Copy callback to avoid use-after-free
            mCurrentCallback = nullptr;
            mCurrentJob = nullptr;
            
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
    if (QCoreApplication::closingDown()) {
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
    
    mCurrentJob = testJob;
    mCurrentAvailabilityCallback = callback;
    
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
            message = qsl("Keychain not available: %1").arg(testJob->errorString());
        }
        
        // Final validity check before calling callback
        if (mCurrentAvailabilityCallback && isOperationValid()) {
            auto callback = mCurrentAvailabilityCallback; // Copy callback to avoid use-after-free
            mCurrentAvailabilityCallback = nullptr;
            mCurrentJob = nullptr;
            
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
    
    return qsl("Mudlet-%1-%2").arg(sanitizedProfile, sanitizedKey);
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
        auto match = pathTraversalPattern.match(profileName);

        if (!match.hasMatch()) {
            match = pathTraversalPattern.match(key);
        }

        QString invalidChar = match.hasMatch() ? match.captured(0) : "unknown";
        qWarning() << "CredentialManager: Invalid characters detected in path components:" << invalidChar;
        return QString();
    }
    
    QString sanitizedProfile = utils::sanitizeForPath(profileName);
    QString sanitizedKey = utils::sanitizeForPath(key);
    
    // QStandardPaths automatically handles portable mode configuration paths
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return qsl("%1/profiles/%2/passwords/%3").arg(configPath, sanitizedProfile, sanitizedKey);
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
    
    connect(readJob, &QKeychain::ReadPasswordJob::finished, this, [readJob, callback, profileName]() {
        bool success = (readJob->error() == QKeychain::NoError);
        QString password = success ? readJob->textData() : QString();
        
        if (success) {
            qDebug() << "CredentialManager: Found legacy password for profile" << profileName;
        } else {
            qDebug() << "CredentialManager: No legacy password found for profile" << profileName;
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
            qDebug() << "CredentialManager: Deleted legacy entry for profile" << profileName;
        } else {
            qDebug() << "CredentialManager: Failed to delete legacy entry for profile" << profileName << ":" << deleteJob->errorString();
        }

        deleteJob->deleteLater();
    });
    
    deleteJob->start();
}
