/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDataStream>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif

// Forward declaration to avoid including mudlet.h
class mudlet;

CredentialManager::CredentialManager(QObject* parent)
: QObject(parent)
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

    // Move callbacks to locals before cleanup — the callback chain may call
    // cleanupCurrentOperation() (e.g. by starting a new keychain operation),
    // which would destroy the std::function while it's still executing,
    // causing use-after-free of captured lambda state
    auto callback = std::exchange(mCurrentCallback, nullptr);
    auto retrievalCallback = std::exchange(mCurrentRetrievalCallback, nullptr);
    auto availabilityCallback = std::exchange(mCurrentAvailabilityCallback, nullptr);

    // Clean up the timed-out operation before invoking the callback,
    // since the callback may start a new operation
    cleanupCurrentOperation();

    if (callback) {
        callback(false, qsl("Operation timed out"));
    } else if (retrievalCallback) {
        retrievalCallback(false, QString(), qsl("Operation timed out"));
    } else if (availabilityCallback) {
        availabilityCallback(false, qsl("Operation timed out"));
    }
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

    if (systemEnvironment.contains(qsl("APPIMAGE"))) {
        QString appimgPath = systemEnvironment.value(qsl("APPIMAGE"), QString());
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

void CredentialManager::storePassword(const QString& profileName, const QString& key, const QString& password, CredentialCallback callback)
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
        storeCredential(service, key, password, profileName, callback);
    } else {
        // Use SecureStringUtils for portable/test environments
        bool success = storeCredentialToFile(profileName, key, password);

        if (callback) {
            callback(success, success ? QString() : qsl("Failed to store password with SecureStringUtils"));
        }
    }
}

void CredentialManager::retrievePassword(const QString& profileName, const QString& key, CredentialRetrievalCallback callback)
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
        // Use keychain storage with fallback chain:
        // 1. New hash-based format (unique per profile name)
        // 2. Windows only: same format under the pre-0.17 qtkeychain naming scheme
        // 3. Old colliding format (profiles with similar names may share passwords)
        // 4. Legacy keychain format (pre-4.20.0)
        // 5. Encrypted file storage
        QString service = generateServiceName(profileName, key);
        QString legacyService = generateLegacyServiceName(profileName, key);

        auto tryCollidingFormat = [this, profileName, key, legacyService, callback]() {
            retrieveCredential(legacyService, key, profileName, [this, profileName, key, legacyService, callback](bool oldSuccess, const QString& oldPassword, const QString& oldError) {
                if (oldSuccess && !oldPassword.isEmpty()) {
                    attemptCollidingMigration(profileName, key, legacyService, oldPassword, callback);
                } else {
                    if (!key.compare(qsl("password")) || !key.compare(qsl("character"))) {
                        attemptLegacyKeychainMigration(profileName, key, callback);
                    } else {
                        fallbackFileRetrieval(profileName, key, callback);
                    }
                }
            });
        };

        auto newFormatCallback = [this, profileName, key, callback, tryCollidingFormat](bool keychainSuccess, const QString& keychainPassword, const QString& keychainError) {
            if (keychainSuccess && !keychainPassword.isEmpty()) {
                if (callback) {
                    callback(true, keychainPassword, QString());
                }
            } else {
                tryCollidingFormat();
            }
        };

        retrieveCredential(service, key, profileName, newFormatCallback);
    } else {
        // Use SecureStringUtils directly (portable/test mode)
        QString password = retrieveCredentialFromFile(profileName, key);
        bool success = !password.isEmpty();

        if (callback) {
            // Empty password is normal for first-time profiles - not an error
            callback(success, password, success ? QString() : qsl("No password stored in encrypted file storage"));
        }
    }
}

void CredentialManager::attemptCollidingMigration(const QString& profileName, const QString& key, const QString& legacyService, const QString& password, CredentialRetrievalCallback callback)
{
    qDebug() << "CredentialManager: Migrating password from colliding format for" << profileName;

    storePassword(profileName, key, password, [this, legacyService, key, profileName, callback, password](bool migrationSuccess, const QString& migrationError) {
        if (migrationSuccess) {
            // Only clean up old colliding entry if version > 4.20.1
            // The colliding format bug existed in 4.20.0 and 4.20.1
            // Preserving the entry allows users to switch back to those versions
#ifdef APP_VERSION
            const QString currentVersion = QString(APP_VERSION);
            QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
            const QVersionNumber collidingFormatVersion = QVersionNumber(4, 20, 1);

            // Dev/test/PTB builds represent the "next release", so bump version for comparison
            QFile buildFile(qsl(":/app-build.txt"));

            if (buildFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QString buildSuffix = QString::fromUtf8(buildFile.readAll()).trimmed();

                if (buildSuffix.startsWith(qsl("-dev")) || buildSuffix.startsWith(qsl("-test")) || buildSuffix.startsWith(qsl("-ptb"))) {
                    appVersion = QVersionNumber(appVersion.majorVersion(), appVersion.minorVersion(), appVersion.microVersion() + 1);
                }
            }

            if (appVersion > collidingFormatVersion) {
                removeCredential(legacyService, key, profileName, [](bool, const QString&) {});
            }
#endif
        } else {
            qWarning() << "CredentialManager: Migration failed:" << migrationError;
        }

        // Return the recovered password even if migration failed
        if (callback) {
            callback(true, password, QString());
        }
    });
}

void CredentialManager::attemptLegacyKeychainMigration(const QString& profileName, const QString& key, CredentialRetrievalCallback callback)
{
    checkLegacyKeychainFormat(profileName, [this, profileName, key, callback](bool legacySuccess, const QString& legacyPassword) {
        if (legacySuccess && !legacyPassword.isEmpty()) {
            qDebug() << "CredentialManager: Migrating password from legacy format for" << profileName;

            storePassword(profileName, key, legacyPassword, [this, profileName, callback, legacyPassword](bool migrationSuccess, const QString& migrationError) {
                if (!migrationSuccess) {
                    qWarning() << "CredentialManager: Migration failed:" << migrationError;
                } else {
                    // Only clean up legacy entry after successful migration if this version is >= 4.20.0
                    // This prevents breaking compatibility with older Mudlet versions
#ifdef APP_VERSION
                    const QString currentVersion = QString(APP_VERSION);
                    const QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
                    const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);

                    if (appVersion >= secureStorageVersion) {
                        deleteLegacyKeychainEntry(profileName);
                    }
#endif
                }

                // Return the recovered password even if migration failed
                if (callback) {
                    callback(true, legacyPassword, QString());
                }
            });
        } else {
            fallbackFileRetrieval(profileName, key, callback);
        }
    });
}

void CredentialManager::fallbackFileRetrieval(const QString& profileName, const QString& key, CredentialRetrievalCallback callback)
{
    QString fallbackPassword = retrieveCredentialFromFile(profileName, key);

    if (callback) {
        if (!fallbackPassword.isEmpty()) {
            callback(true, fallbackPassword, QString());
        } else {
            callback(false, QString(), qsl("No stored credentials found for profile %1").arg(profileName));
        }
    }
}

void CredentialManager::attemptOldFormatMigration(const QString& service, const QString& account, const QString& profileName, CredentialRetrievalCallback callback)
{
    // Before the Windows keychain fix, credentials were stored with key=account instead of key=service
    // On Windows, only setKey() value is used as the TargetName, causing all profiles to share "character"
    // This function tries to read from the old format and migrate to the new format
    qDebug() << "CredentialManager: Checking for old format (key=account) for service:" << service;

#if defined(Q_OS_WIN)
    // Old-format entries live at TargetName == account; qtkeychain 0.17+ would look up
    // "account@service" and miss them, while an empty service resolves to the bare key on
    // every qtkeychain version (pre-0.17 ignores the service entirely)
    const QString lookupService;
#else
    const QString lookupService = service;
#endif

    auto* oldFormatJob = new QKeychain::ReadPasswordJob(lookupService, this);
    oldFormatJob->setKey(account); // Old format used account as key
    oldFormatJob->setAutoDelete(false);

    connect(oldFormatJob, &QKeychain::ReadPasswordJob::finished, this, [this, oldFormatJob, service, lookupService, account, profileName, callback]() {
        bool found = (oldFormatJob->error() == QKeychain::NoError);
        QString password = found ? oldFormatJob->textData() : QString();

        if (found && !password.isEmpty()) {
            qDebug() << "CredentialManager: Found password in old format, migrating to new format";

            // Store in new format (key=service) for future use
            auto* migrateJob = new QKeychain::WritePasswordJob(service, this);
            migrateJob->setKey(service); // New format
            migrateJob->setTextData(password);
            migrateJob->setAutoDelete(false);

            connect(migrateJob, &QKeychain::WritePasswordJob::finished, this, [migrateJob, service, lookupService, account, password, callback]() {
                if (migrateJob->error() == QKeychain::NoError) {
                    qDebug() << "CredentialManager: Migration to new format successful, cleaning up old entry";

                    auto* cleanupJob = new QKeychain::DeletePasswordJob(lookupService);
                    cleanupJob->setKey(account); // Old format key
                    cleanupJob->setAutoDelete(true);
                    connect(cleanupJob, &QKeychain::DeletePasswordJob::finished, cleanupJob, [cleanupJob, service]() {
                        if (cleanupJob->error() == QKeychain::NoError || cleanupJob->error() == QKeychain::EntryNotFound) {
                            qDebug() << "CredentialManager: Old format entry cleaned up for service:" << service;
                        } else {
                            qWarning() << "CredentialManager: Failed to clean up old format entry for service:" << service << "-" << cleanupJob->errorString();
                        }
                    });
                    cleanupJob->start();
                } else {
                    qWarning() << "CredentialManager: Migration to new format failed:" << migrateJob->errorString();
                }

                // Return password regardless of migration success
                if (callback) {
                    callback(true, password, QString());
                }
                migrateJob->deleteLater();
            });

            migrateJob->start();
        } else {
            qDebug() << "CredentialManager: Old format not found, trying other fallbacks";
            // Continue with existing legacy format checks
            // The legacy "Mudlet profile" keychain format was only used for character and password keys
            if (!account.compare(qsl("character")) || !account.compare(qsl("password"))) {
                attemptLegacyKeychainMigration(profileName, account, callback);
            } else {
                fallbackFileRetrieval(profileName, account, callback);
            }
        }

        oldFormatJob->deleteLater();
    });

    oldFormatJob->start();
}

void CredentialManager::attemptCompatNamingMigration(const QString& service, const QString& account, const QString& profileName, CredentialRetrievalCallback callback)
{
    // qtkeychain 0.17.0 changed the Windows Credential Manager TargetName from the bare key to
    // "key@service", so entries stored by builds linked against an older qtkeychain (which used
    // TargetName == key == service) are no longer found by the primary read. A read with an empty
    // service resolves to TargetName == key on every qtkeychain version - pre-0.17 ignores the
    // service and 0.17+ falls back to the bare key - so it recovers those entries on both. On
    // pre-0.17 this normally stays dormant (the primary read already looks up the bare key), but
    // a transient primary-read failure can still route here, where the compat read is merely a
    // retry of the same TargetName.
    qDebug() << "CredentialManager: Checking for pre-0.17 qtkeychain naming for service:" << service;
#if defined(QTKEYCHAIN_LINKED_VERSION)
    qDebug() << "CredentialManager: Linked qtkeychain version:" << QTKEYCHAIN_LINKED_VERSION;
#endif

    auto* compatJob = new QKeychain::ReadPasswordJob(QString(), this);
    compatJob->setKey(service);
    compatJob->setAutoDelete(false);

    connect(compatJob, &QKeychain::ReadPasswordJob::finished, this, [this, compatJob, service, account, profileName, callback]() {
        const bool found = (compatJob->error() == QKeychain::NoError);
        const QString password = found ? compatJob->textData() : QString();

        if (found && !password.isEmpty()) {
            qDebug() << "CredentialManager: Found password under pre-0.17 naming, migrating to current scheme";

            // Re-store through a normal write (key == service) so the entry lands under the
            // naming scheme of the linked qtkeychain version
            auto* migrateJob = new QKeychain::WritePasswordJob(service, this);
            migrateJob->setKey(service);
            migrateJob->setTextData(password);
            migrateJob->setAutoDelete(false);

            connect(migrateJob, &QKeychain::WritePasswordJob::finished, this, [migrateJob, service, password, callback]() {
                if (migrateJob->error() == QKeychain::NoError) {
                    qDebug() << "CredentialManager: Migration to current naming successful";

                    // On pre-0.17 qtkeychain the write above resolves to the same bare TargetName
                    // as the old entry, so deleting it would remove the credential that was just
                    // restored - only clean up when the linked qtkeychain uses the new naming scheme
#if defined(QTKEYCHAIN_LINKED_VERSION)
                    if (QVersionNumber::fromString(qsl(QTKEYCHAIN_LINKED_VERSION)) >= QVersionNumber(0, 17, 0)) {
                        qDebug() << "CredentialManager: Cleaning up pre-0.17 entry";

                        auto* cleanupJob = new QKeychain::DeletePasswordJob(QString());
                        cleanupJob->setKey(service);
                        cleanupJob->setAutoDelete(true);
                        connect(cleanupJob, &QKeychain::DeletePasswordJob::finished, cleanupJob, [cleanupJob, service]() {
                            if (cleanupJob->error() == QKeychain::NoError || cleanupJob->error() == QKeychain::EntryNotFound) {
                                qDebug() << "CredentialManager: Pre-0.17 entry cleaned up for service:" << service;
                            } else {
                                qWarning() << "CredentialManager: Failed to clean up pre-0.17 entry for service:" << service << "-" << cleanupJob->errorString();
                            }
                        });
                        cleanupJob->start();
                    }
#endif
                } else {
                    qWarning() << "CredentialManager: Migration to current naming failed:" << migrateJob->errorString();
                }

                // Return password regardless of migration success
                if (callback) {
                    callback(true, password, QString());
                }
                migrateJob->deleteLater();
            });

            migrateJob->start();
        } else {
            qDebug() << "CredentialManager: No pre-0.17 entry found, trying other fallbacks";
            attemptOldFormatMigration(service, account, profileName, callback);
        }

        compatJob->deleteLater();
    });

    compatJob->start();
}

void CredentialManager::removePassword(const QString& profileName, const QString& key, CredentialCallback callback)
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
        QString service = generateServiceName(profileName, key);
        QString legacyService = generateLegacyServiceName(profileName, key);

        auto combinedCallback = [this, profileName, key, legacyService, callback](bool keychainSuccess, const QString& keychainError) {
            removeCredential(legacyService, key, profileName, [this, profileName, key, callback, keychainSuccess, keychainError](bool oldSuccess, const QString&) {
                bool fileSuccess = removeCredentialFromFile(profileName, key);

                if (callback) {
                    if (keychainSuccess || oldSuccess || fileSuccess) {
                        callback(true, QString());
                    } else {
                        callback(false, qsl("Failed to remove from keychain: %1").arg(keychainError));
                    }
                }
            });
        };

        removeCredential(service, key, profileName, combinedCallback);
    } else {
        // Use SecureStringUtils
        bool success = removeCredentialFromFile(profileName, key);

        if (callback) {
            callback(success, success ? QString() : qsl("Failed to remove password with SecureStringUtils"));
        }
    }
}

void CredentialManager::migratePassword(const QString& profileName, const QString& key, const QString& plaintextPassword, CredentialCallback callback)
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

void CredentialManager::storeCredential(const QString& service, const QString& account, const QString& password, const QString& profileName, CredentialCallback callback)
{
    if (service.isEmpty() || account.isEmpty() || profileName.isEmpty()) {
        if (callback) {
            callback(false, qsl("Service, account, and profile name cannot be empty"));
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
    // Use service as the key - on Windows, only setKey() value is used as the credential target,
    // so using account ("character") would cause all profiles to share the same credential
    writeJob->setKey(service);

    // Store password directly in keychain (keychain handles encryption)
    writeJob->setTextData(password);
    writeJob->setAutoDelete(false);

    mCurrentJob = writeJob;
    mCurrentCallback = callback;

    // Set up timeout
    setupTimeout();

    // Connect signals with queued connection for safety
    connect(
            writeJob,
            &QKeychain::WritePasswordJob::finished,
            this,
            [this, writeJob, service, account, password, profileName]() {
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

                    bool fileSuccess = storeCredentialToFile(profileName, account, password);

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
            },
            Qt::QueuedConnection); // Use queued connection for additional safety

    writeJob->start();
}

void CredentialManager::retrieveCredential(const QString& service, const QString& account, const QString& profileName, CredentialRetrievalCallback callback)
{
    if (service.isEmpty() || account.isEmpty() || profileName.isEmpty()) {
        if (callback) {
            callback(false, QString(), qsl("Service, account, and profile name cannot be empty"));
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
    // Use service as the key - on Windows, only setKey() value is used as the credential target,
    // so using account ("character") would cause all profiles to share the same credential
    readJob->setKey(service);
    readJob->setAutoDelete(false);

    mCurrentJob = readJob;
    mCurrentRetrievalCallback = callback;

    // Set up timeout
    setupTimeout();

    // Connect signals with queued connection for safety
    connect(
            readJob,
            &QKeychain::ReadPasswordJob::finished,
            this,
            [this, readJob, service, account, profileName]() {
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
                        errorContext = qsl("No password stored in keychain");
                        break;
                    case QKeychain::AccessDeniedByUser:
                        errorContext = qsl("User denied keychain access");
                        break;
                    case QKeychain::AccessDenied:
                        errorContext = qsl("Keychain access denied by system");
                        break;
                    case QKeychain::NoBackendAvailable:
                        errorContext = qsl("No keychain service available");
                        break;
                    case QKeychain::NotImplemented:
                        errorContext = qsl("Keychain not supported on this platform");
                        break;
                    default:
                        errorContext = qsl("Keychain error: %1").arg(readJob->errorString());
                        break;
                    }
                    qDebug() << "CredentialManager:" << errorContext << ", trying fallback storage";

                    // Try old format first (before Windows keychain fix), then legacy formats
                    // Clear state to prevent callback being called twice and keep member state consistent
                    auto originalCallback = mCurrentRetrievalCallback;
                    mCurrentRetrievalCallback = nullptr;
                    mCurrentJob = nullptr;
#if defined(Q_OS_WIN)
                    // qtkeychain 0.17.0 started honouring the service name on Windows, moving
                    // entries from TargetName "<key>" to "<key>@<service>" - recover entries
                    // written by builds linked against older qtkeychain first
                    attemptCompatNamingMigration(service, account, profileName, originalCallback);
#else
                    attemptOldFormatMigration(service, account, profileName, originalCallback);
#endif
                    readJob->deleteLater();
                    return;
                }

                // Final validity check before calling callback
                if (mCurrentRetrievalCallback && isOperationValid()) {
                    auto callback = mCurrentRetrievalCallback; // Copy callback to avoid use-after-free
                    mCurrentRetrievalCallback = nullptr;
                    mCurrentJob = nullptr;

                    callback(success, password, errorMessage);
                }

                readJob->deleteLater();
            },
            Qt::QueuedConnection); // Use queued connection for additional safety

    readJob->start();
}

void CredentialManager::removeCredential(const QString& service, const QString& account, const QString& profileName, CredentialCallback callback)
{
    if (service.isEmpty() || account.isEmpty() || profileName.isEmpty()) {
        if (callback) {
            callback(false, "Service, account, and profile name cannot be empty");
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
    // Use service as the key - on Windows, only setKey() value is used as the credential target,
    // so using account ("character") would cause all profiles to share the same credential
    deleteJob->setKey(service);
    deleteJob->setAutoDelete(false);

    mCurrentJob = deleteJob;
    mCurrentCallback = callback;

    // Set up timeout
    setupTimeout();

    // Connect signals with queued connection for safety
    connect(
            deleteJob,
            &QKeychain::DeletePasswordJob::finished,
            this,
            [this, deleteJob, service, account, profileName]() {
                // Early exit if operation is no longer valid
                if (!isOperationValid()) {
                    qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
                    deleteJob->deleteLater();
                    return;
                }

                cleanupTimeout();

                bool keychainSuccess = (deleteJob->error() == QKeychain::NoError || deleteJob->error() == QKeychain::EntryNotFound);

                bool fileSuccess = removeCredentialFromFile(profileName, account);

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
            },
            Qt::QueuedConnection); // Use queued connection for additional safety

    deleteJob->start();

#if defined(Q_OS_WIN)
    // A pre-0.17 bare entry (TargetName == service) may still exist if it was written by a build
    // linked against an older qtkeychain and no read has migrated it yet; on 0.17+ the delete
    // above resolves to "service@service" and would leave that copy behind to be resurrected by
    // the compat migration on a later read. Sweep the bare name too - on pre-0.17 both deletes
    // resolve to the same TargetName, so this is a harmless no-op there.
    auto* bareCleanupJob = new QKeychain::DeletePasswordJob(QString());
    bareCleanupJob->setKey(service);
    bareCleanupJob->setAutoDelete(true);
    connect(bareCleanupJob, &QKeychain::DeletePasswordJob::finished, bareCleanupJob, [bareCleanupJob, service]() {
        if (bareCleanupJob->error() != QKeychain::NoError && bareCleanupJob->error() != QKeychain::EntryNotFound) {
            qWarning() << "CredentialManager: Failed to remove pre-0.17 entry for service:" << service << "-" << bareCleanupJob->errorString();
        }
    });
    bareCleanupJob->start();
#endif
}

void CredentialManager::isKeychainAvailable(AvailabilityCallback callback)
{
    if (!callback) {
        return;
    }

    // Check if we're in test environment
    if (SecureStringUtils::isTestEnvironment()) {
        callback(false, qsl("Keychain disabled in test environment"));
        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting isKeychainAvailable operation during shutdown";
        callback(false, qsl("Application is shutting down"));
        return;
    }

    // Cleanup any existing operation
    cleanupCurrentOperation();

    // Test keychain availability by trying to read a non-existent key
    auto* testJob = new QKeychain::ReadPasswordJob(qsl("MudletKeychainTest"), this);
    testJob->setKey(qsl("availability_test"));
    testJob->setAutoDelete(false);

    mCurrentJob = testJob;
    mCurrentAvailabilityCallback = callback;

    // Set up timeout
    setupTimeout();

    // Connect signals with queued connection for safety
    connect(
            testJob,
            &QKeychain::ReadPasswordJob::finished,
            this,
            [this, testJob]() {
                // Early exit if operation is no longer valid
                if (!isOperationValid()) {
                    qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
                    testJob->deleteLater();
                    return;
                }

                cleanupTimeout();

                bool available = true;
                QString message = qsl("Keychain is available");

                // Check for specific errors that indicate keychain is not available
                if (testJob->error() == QKeychain::AccessDenied || testJob->error() == QKeychain::OtherError) {
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
            },
            Qt::QueuedConnection); // Use queued connection for additional safety

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
    // Regular Expression to detect unwanted characters:
    static const auto sanitizeCharacterPattern = QRegularExpression(qsl(R"REGEX([^\w\-\.])REGEX"));

    // Use SHA-256 hash to generate unique service names that won't collide
    // even when profile names differ only in special characters
    QByteArray data = qsl("%1:%2").arg(profileName, key).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString hashHex = QString::fromLatin1(hash.toHex()).left(16);

    // Include a readable prefix with sanitized profile name and key for easier keychain debugging
    auto sanitizeForDisplay = [](const QString& input) -> QString {
        QString sanitized = input;
        sanitized.replace(sanitizeCharacterPattern, qsl("_"));
        if (sanitized.length() > 20) {
            sanitized = sanitized.left(20);
        }
        return sanitized;
    };

    QString sanitizedProfile = sanitizeForDisplay(profileName);
    QString sanitizedKey = sanitizeForDisplay(key);
    return qsl("Mudlet-%1-%2-%3").arg(sanitizedProfile, sanitizedKey, hashHex);
}

QString CredentialManager::generateLegacyServiceName(const QString& profileName, const QString& key)
{
    // Regular Expression to detect unwanted characters:
    static const auto sanitizeCharacterPattern = QRegularExpression(qsl(R"REGEX([^\w\-\.])REGEX"));

    // Original service name generation that caused collisions
    // Kept for backwards compatibility to migrate existing passwords
    auto sanitizeForService = [](const QString& input) -> QString {
        QString sanitized = input;
        sanitized.replace(sanitizeCharacterPattern, qsl("_"));
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
    static const QRegularExpression pathTraversalPattern(qsl(R"REGEX(\.\.|[<>:"|?*\x00-\x1f])REGEX"));

    if (profileName.contains(pathTraversalPattern) || key.contains(pathTraversalPattern)) {
        auto match = pathTraversalPattern.match(profileName);

        if (!match.hasMatch()) {
            match = pathTraversalPattern.match(key);
        }

        QString invalidChar = match.hasMatch() ? match.captured(0) : qsl("unknown");
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
    static const QRegularExpression dangerousPattern(qsl(R"REGEX(\.\.|[<>:"|?*\x00-\x1f/\\])REGEX"));
    return !key.contains(dangerousPattern);
}

void CredentialManager::checkLegacyKeychainFormat(const QString& profileName, std::function<void(bool, const QString&)> callback)
{
    if (profileName.isEmpty() || !callback) {
        if (callback) {
            callback(false, QString());
        }

        return;
    }

    // Legacy format used service="Mudlet profile" and key=profileName
    const QString legacyService = qsl("Mudlet profile");

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
    const QString legacyService = qsl("Mudlet profile");

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
