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
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include "../3rdparty/qtkeychain/keychain.h"
#else
#include <qt6keychain/keychain.h>
#endif
#include "post_guard.h"

// Public API for secure credential storage with QtKeychain and encrypted fallback
bool CredentialManager::storeCredential(const QString& profileName, const QString& key, const QString& credential)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }
    
    // Empty credential is valid - it means "no credential"
    if (credential.isEmpty()) {
        return removeCredential(profileName, key);
    }
    
    // Skip keychain in test environment
    if (isTestEnvironment()) {
        return storeEncrypted(profileName, key, credential);
    }
    
    // Try QtKeychain first
    if (storeInKeychain(profileName, key, credential)) {
        // Success! Also store an encrypted backup for timeout fallback scenarios
        // This ensures we have a fallback if keychain access times out later
        if (!storeEncrypted(profileName, key, credential)) {
            qWarning() << "CredentialManager: Failed to create encrypted backup for profile" << profileName;
            // Still return true since keychain storage succeeded
        }
        return true;
    }
    
    // If keychain storage failed (possibly due to timeout), warn and use encrypted fallback
    qWarning() << "CredentialManager: Keychain storage failed for profile" << profileName 
               << "- using encrypted fallback (keychain access may have timed out)";
    
    // Fallback to encrypted storage for portable mode or when keychain fails
    return storeEncrypted(profileName, key, credential);
}

QString CredentialManager::retrieveCredential(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return QString();
    }
    
    // Skip keychain in test environment
    if (isTestEnvironment()) {
        return retrieveEncrypted(profileName, key);
    }
    
    // Try QtKeychain first
    QString credential = retrieveFromKeychain(profileName, key);

    // If we got a credential from keychain, use it
    if (!credential.isEmpty()) {
        QString result = credential;
        SecureStringUtils::secureStringClear(credential);
        return result;
    }
    
    // If keychain returned empty, try encrypted fallback
    QString fallbackCredential = retrieveEncrypted(profileName, key);
    
    // If we have a fallback credential, this suggests keychain access may have timed out
    // Only warn if we actually found a fallback (indicating the credential exists but keychain failed)
    if (!fallbackCredential.isEmpty()) {
        qDebug() << "CredentialManager: Using encrypted fallback for profile" << profileName 
                 << "- keychain access may have timed out or been canceled";
    }
    
    return fallbackCredential;
}

bool CredentialManager::removeCredential(const QString& profileName, const QString& key)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        return false;
    }
    
    bool keychainRemoved = true; // Assume success in test environment
    const bool encryptedRemoved = removeEncrypted(profileName, key);
    
    // Skip keychain in test environment
    if (!isTestEnvironment()) {
        keychainRemoved = removeFromKeychain(profileName, key);
    }
    
    // Success if at least one method succeeded
    // Both should be removed since we now store backups in both locations
    return keychainRemoved && encryptedRemoved;
}

bool CredentialManager::isKeychainAvailable()
{
    // Test keychain availability by attempting to read a non-existent key
    auto *job = new QKeychain::ReadPasswordJob(qsl("MudletKeychainTest"));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(qsl("NonExistentTestKey"));
    
    // Use heap-allocated event loop to avoid use-after-free when callback fires after function scope ends
    auto *loop = new QEventLoop();
    // Use heap-allocated boolean to avoid race conditions
    auto *available = new bool(false);
    
    // Add timeout protection to prevent infinite blocking
    // Use heap-allocated timer to avoid use-after-free when timeout fires after function scope ends
    auto *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(5000); // 5 second timeout
    
    QObject::connect(timeoutTimer, &QTimer::timeout, [available, loop, timeoutTimer]() {
        *available = false; // Assume keychain unavailable on timeout
        timeoutTimer->deleteLater(); // Clean up timer
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [available, loop, timeoutTimer](QKeychain::Job* task) {
        timeoutTimer->stop(); // Safe to call - timer is heap-allocated
        timeoutTimer->deleteLater(); // Clean up timer
        // Keychain is available if we get NoError or EntryNotFound (service exists)
        *available = (task->error() == QKeychain::NoError || 
                     task->error() == QKeychain::EntryNotFound);
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    timeoutTimer->start();
    job->start();
    loop->exec();
    job->deleteLater();
    
    bool result = *available;
    delete available; // Clean up heap-allocated boolean
    return result;
}

bool CredentialManager::storeInKeychain(const QString& profileName, const QString& key, const QString& credential)
{
    auto *job = new QKeychain::WritePasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    job->setTextData(credential);
    
    // Use heap-allocated event loop to avoid use-after-free when callback fires after function scope ends
    auto *loop = new QEventLoop();
    // Use heap-allocated boolean to avoid race conditions
    auto *success = new bool(false);
    
    // Add timeout protection to prevent infinite blocking
    // Use heap-allocated timer to avoid use-after-free when timeout fires after function scope ends
    auto *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(30000); // 30 second timeout - more reasonable for user keychain interaction
    
    // Store connections so we can disconnect them to prevent double execution
    QMetaObject::Connection timeoutConnection;
    QMetaObject::Connection finishedConnection;
    
    timeoutConnection = QObject::connect(timeoutTimer, &QTimer::timeout, [success, loop, timeoutTimer, job, &finishedConnection]() {
        // Disconnect the finished signal to prevent it from running after timeout
        QObject::disconnect(finishedConnection);
        
        *success = false; // Assume failure on timeout
        timeoutTimer->deleteLater(); // Clean up timer
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    finishedConnection = QObject::connect(job, &QKeychain::WritePasswordJob::finished, [success, loop, timeoutTimer, &timeoutConnection](QKeychain::Job* task) {
        // Disconnect the timeout signal to prevent it from running after completion
        QObject::disconnect(timeoutConnection);
        
        timeoutTimer->stop(); // Safe to call - timer is heap-allocated
        timeoutTimer->deleteLater(); // Clean up timer
        *success = !task->error();
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    timeoutTimer->start();
    job->start();
    loop->exec();
    job->deleteLater();
    
    bool result = *success;
    delete success; // Clean up heap-allocated boolean
    return result;
}

QString CredentialManager::retrieveFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::ReadPasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    // Use heap-allocated event loop to avoid use-after-free when callback fires after function scope ends
    auto *loop = new QEventLoop();
    // Use heap-allocated credential to avoid race conditions
    auto *credential = new QString();
    // Use heap-allocated result to safely pass data back
    auto *result = new QString();
    
    // Add timeout protection to prevent infinite blocking
    // Use heap-allocated timer to avoid use-after-free when timeout fires after function scope ends
    auto *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(30000); // 30 second timeout - more reasonable for user keychain interaction
    
    // Store connections so we can disconnect them to prevent double execution
    QMetaObject::Connection timeoutConnection;
    QMetaObject::Connection finishedConnection;
    
    timeoutConnection = QObject::connect(timeoutTimer, &QTimer::timeout, [loop, timeoutTimer, credential, result, job, &finishedConnection]() {
        // Disconnect the finished signal to prevent it from running after timeout
        QObject::disconnect(finishedConnection);
        
        // On timeout, do NOT delete stored credentials - just return empty result
        // The credential may exist in keychain but user didn't respond to prompt in time
        timeoutTimer->deleteLater(); // Clean up timer
        SecureStringUtils::secureStringClear(*credential);
        delete credential; // Clean up credential (this is just the local variable, not keychain storage)
        // result remains empty (default constructed) to indicate timeout/failure
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    finishedConnection = QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [credential, loop, timeoutTimer, result, &timeoutConnection](QKeychain::Job* task) {
        // Disconnect the timeout signal to prevent it from running after completion
        QObject::disconnect(timeoutConnection);
        
        timeoutTimer->stop(); // Safe to call - timer is heap-allocated
        timeoutTimer->deleteLater(); // Clean up timer
        if (!task->error()) {
            auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
            *credential = readJob->textData();
        }
        // Move credential to result and securely clear
        *result = std::move(*credential);
        SecureStringUtils::secureStringClear(*credential);
        delete credential; // Clean up credential
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    timeoutTimer->start();
    job->start();
    loop->exec();
    job->deleteLater();
    
    // Get result and clean up
    QString finalResult = std::move(*result);
    delete result; // Clean up result
    return finalResult;
}

bool CredentialManager::removeFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::DeletePasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    // Use heap-allocated event loop to avoid use-after-free when callback fires after function scope ends
    auto *loop = new QEventLoop();
    // Use heap-allocated boolean to avoid race conditions
    auto *success = new bool(false);
    
    // Add timeout protection to prevent infinite blocking
    // Use heap-allocated timer to avoid use-after-free when timeout fires after function scope ends
    auto *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(30000); // 30 second timeout - more reasonable for user keychain interaction
    
    // Store connections so we can disconnect them to prevent double execution
    QMetaObject::Connection timeoutConnection;
    QMetaObject::Connection finishedConnection;
    
    timeoutConnection = QObject::connect(timeoutTimer, &QTimer::timeout, [success, loop, timeoutTimer, job, &finishedConnection]() {
        // Disconnect the finished signal to prevent it from running after timeout
        QObject::disconnect(finishedConnection);
        
        *success = false; // Assume failure on timeout
        timeoutTimer->deleteLater(); // Clean up timer
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    finishedConnection = QObject::connect(job, &QKeychain::DeletePasswordJob::finished, [success, loop, timeoutTimer, &timeoutConnection](QKeychain::Job* task) {
        // Disconnect the timeout signal to prevent it from running after completion
        QObject::disconnect(timeoutConnection);
        
        timeoutTimer->stop(); // Safe to call - timer is heap-allocated
        timeoutTimer->deleteLater(); // Clean up timer
        *success = !task->error() || task->error() == QKeychain::EntryNotFound;
        loop->quit();
        loop->deleteLater(); // Clean up event loop
    });
    
    timeoutTimer->start();
    job->start();
    loop->exec();
    job->deleteLater();
    
    bool result = *success;
    delete success; // Clean up heap-allocated boolean
    return result;
}

bool CredentialManager::storeEncrypted(const QString& profileName, const QString& key, const QString& credential)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Check if path generation failed due to validation
    if (filePath.isEmpty()) {
        qWarning() << "CredentialManager: Failed to generate valid file path for storing encrypted credential";
        return false;
    }
    
    // Validate credential input
    if (credential.isEmpty()) {
        qWarning() << "CredentialManager: Empty credential provided for storage";
        return false;
    }
    
    // Create directory structure if it doesn't exist
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();

    if (!dir.mkpath(dir.absolutePath())) {
        qWarning() << "CredentialManager: Failed to create directory structure for" << filePath;
        return false;
    }
    
    // Encrypt credential using profile-specific key
    QString encrypted = SecureStringUtils::encryptStringForProfile(credential, profileName);

    if (encrypted.isEmpty()) {
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

QString CredentialManager::retrieveEncrypted(const QString& profileName, const QString& key)
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

bool CredentialManager::removeEncrypted(const QString& profileName, const QString& key)
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

bool CredentialManager::isTestEnvironment()
{
    // Detect test environment to avoid keychain prompts during automated testing
    
    // Check environment variables and application context
    QString appName = QCoreApplication::applicationName();
    QStringList args = QCoreApplication::arguments();
    
    return qEnvironmentVariableIsSet("MUDLET_TEST_MODE") ||
           appName.contains("Test", Qt::CaseInsensitive) ||
           args.first().contains("Test", Qt::CaseInsensitive);
}
