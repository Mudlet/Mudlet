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
#include <QStandardPaths>
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
        // Also remove any encrypted fallback file if it exists
        removeEncrypted(profileName, key);
        return true;
    }
    
    // Fallback to encrypted storage for portable mode
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

    if (!credential.isNull()) {
        QString result = credential;
        SecureStringUtils::secureStringClear(credential);
        return result;
    }
    
    // Fallback to encrypted storage
    return retrieveEncrypted(profileName, key);
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
    
    // Success if at least one method succeeded (or both failed because nothing was stored)
    return keychainRemoved || encryptedRemoved;
}

bool CredentialManager::isKeychainAvailable()
{
    // Test keychain availability by attempting to read a non-existent key
    auto *job = new QKeychain::ReadPasswordJob(qsl("MudletKeychainTest"));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(qsl("NonExistentTestKey"));
    
    QEventLoop loop;
    bool available = false;
    
    QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [&](QKeychain::Job* task) {
        // Keychain is available if we get NoError or EntryNotFound (service exists)
        available = (task->error() == QKeychain::NoError || 
                    task->error() == QKeychain::EntryNotFound);
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return available;
}

bool CredentialManager::storeInKeychain(const QString& profileName, const QString& key, const QString& credential)
{
    auto *job = new QKeychain::WritePasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    job->setTextData(credential);
    
    QEventLoop loop;
    bool success = false;
    
    QObject::connect(job, &QKeychain::WritePasswordJob::finished, [&](QKeychain::Job* task) {
        success = !task->error();
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return success;
}

QString CredentialManager::retrieveFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::ReadPasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    QEventLoop loop;
    QString credential;
    
    QObject::connect(job, &QKeychain::ReadPasswordJob::finished, [&](QKeychain::Job* task) {
        if (!task->error()) {
            auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
            credential = readJob->textData();
        }
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    QString result = credential;
    SecureStringUtils::secureStringClear(credential);
    return result;
}

bool CredentialManager::removeFromKeychain(const QString& profileName, const QString& key)
{
    auto *job = new QKeychain::DeletePasswordJob(generateServiceName(profileName, key));

    job->setAutoDelete(false);
    job->setInsecureFallback(false);
    job->setKey(profileName);
    
    QEventLoop loop;
    bool success = false;
    
    QObject::connect(job, &QKeychain::DeletePasswordJob::finished, [&](QKeychain::Job* task) {
        success = !task->error() || task->error() == QKeychain::EntryNotFound;
        loop.quit();
    });
    
    job->start();
    loop.exec();
    job->deleteLater();
    
    return success;
}

bool CredentialManager::storeEncrypted(const QString& profileName, const QString& key, const QString& credential)
{
    QString filePath = generateFilePath(profileName, key);
    
    // Create directory structure if it doesn't exist
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();

    if (!dir.mkpath(dir.absolutePath())) {
        return false;
    }
    
    // Encrypt credential using profile-specific key
    QString encrypted = SecureStringUtils::encryptStringForProfile(credential, profileName);

    if (encrypted.isEmpty()) {
        return false;
    }
    
    // Write to file
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    file.write(encrypted.toUtf8());
    file.close();
    
    return true;
}

QString CredentialManager::retrieveEncrypted(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString encrypted = QString::fromUtf8(file.readAll());
    file.close();
    
    if (encrypted.isEmpty()) {
        return QString();
    }
    
    // Decrypt credential using profile-specific key
    return SecureStringUtils::decryptStringForProfile(encrypted, profileName);
}

bool CredentialManager::removeEncrypted(const QString& profileName, const QString& key)
{
    QString filePath = generateFilePath(profileName, key);
    return QFile::remove(filePath);
}

QString CredentialManager::generateServiceName(const QString& profileName, const QString& key)
{
    return QString("Mudlet-%1-%2").arg(profileName, key);
}

QString CredentialManager::generateFilePath(const QString& profileName, const QString& key)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QString("%1/profiles/%2/passwords/%3").arg(configPath, profileName, key);
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
