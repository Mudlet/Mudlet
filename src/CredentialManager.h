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

#ifndef MUDLET_CREDENTIALMANAGER_H
#define MUDLET_CREDENTIALMANAGER_H

#include "pre_guard.h"
#include <QObject>
#include <QString>
#include <QPointer>
#include <functional>
#include "post_guard.h"

class QTimer;

namespace QKeychain {
class Job;
class ReadPasswordJob;
class WritePasswordJob;
class DeletePasswordJob;
}

/**
 * @brief Secure credential management with QtKeychain integration and encrypted file fallback
 * 
 * This class provides a comprehensive credential management system following the principle:
 * "QtKeychain first, encrypted file fallback". It offers both asynchronous and legacy APIs.
 * 
 * RECOMMENDED: Async API (QtKeychain + fallback)
 * - Primary storage: System keychain (macOS Keychain, Windows Credential Store, Linux Secret Service)
 * - Automatic fallback: AES-256 encrypted files when keychain unavailable
 * - Non-blocking operations with callback-based results
 * - Better security and user experience
 * 
 * LEGACY: Static API (file storage only)
 * - Encrypted file storage only (no keychain integration)
 * - Synchronous operations for backwards compatibility
 * - Consider migrating to async API for better security
 * 
 * Features:
 * - Per-profile credential isolation
 * - Timeout protection and resource cleanup  
 * - Input validation and sanitization
 * - Test environment detection
 * - Cross-platform compatibility
 */
class CredentialManager : public QObject
{
    Q_OBJECT

public:
    explicit CredentialManager(QObject* parent = nullptr);
    ~CredentialManager();

    // Callback types for asynchronous operations
    using CredentialCallback = std::function<void(bool success, const QString& errorMessage)>;
    using CredentialRetrievalCallback = std::function<void(bool success, const QString& password, const QString& errorMessage)>;
    using AvailabilityCallback = std::function<void(bool available, const QString& message)>;

    // Asynchronous methods for credential management (preferred)
    void storeCredential(const QString& service, const QString& account, const QString& password, CredentialCallback callback);
    void retrieveCredential(const QString& service, const QString& account, CredentialRetrievalCallback callback);
    void removeCredential(const QString& service, const QString& account, CredentialCallback callback);
    
    // Check if QtKeychain is available and working (asynchronous)
    void isKeychainAvailable(AvailabilityCallback callback);

    // Hybrid password management methods (preferred)
    // These methods intelligently choose between keychain and SecureStringUtils based on availability and portable mode
    void storePassword(const QString& profileName, const QString& key, const QString& password, CredentialCallback callback);
    void retrievePassword(const QString& profileName, const QString& key, CredentialRetrievalCallback callback);
    void removePassword(const QString& profileName, const QString& key, CredentialCallback callback);
    
    // Password migration method - migrates plaintext passwords to encrypted storage
    void migratePassword(const QString& profileName, const QString& key, const QString& plaintextPassword, CredentialCallback callback);

    // Static fallback methods (for migration only - uses encrypted file storage)
    static bool storeCredential(const QString& profileName, const QString& key, const QString& credential);
    static QString retrieveCredential(const QString& profileName, const QString& key);
    static bool removeCredential(const QString& profileName, const QString& key);

private:
    static constexpr int OPERATION_TIMEOUT_MS = 30000; // 30 seconds
    
    // Portable mode detection
    bool isPortableModeActive() const;
    bool shouldUseKeychain(const QString& profileName) const;
    
    // Timeout and cleanup management
    void setupTimeout();
    void cleanupTimeout();
    void handleTimeout();
    void cleanupCurrentOperation();
    
    // Safety guard for keychain operation callbacks
    bool isOperationValid() const;
    
    // Static utility methods for fallback storage
    static QString generateFilePath(const QString& profileName, const QString& key);
    static QString generateServiceName(const QString& profileName, const QString& key);
    static bool isValidKeyName(const QString& key);
    static bool storeCredentialToFile(const QString& profileName, const QString& key, const QString& credential);
    static QString retrieveCredentialFromFile(const QString& profileName, const QString& key);
    static bool removeCredentialFromFile(const QString& profileName, const QString& key);
    
    // Legacy keychain migration support
    void checkLegacyKeychainFormat(const QString& profileName, std::function<void(bool, const QString&)> callback);
    void deleteLegacyKeychainEntry(const QString& profileName);
    
    // Current operation state
    QPointer<QKeychain::Job> mCurrentJob{nullptr};
    QTimer* mTimeoutTimer{nullptr};
    CredentialCallback mCurrentCallback;
    CredentialRetrievalCallback mCurrentRetrievalCallback;
    AvailabilityCallback mCurrentAvailabilityCallback;
    
    // Destruction flag to prevent operations during cleanup
    bool mShuttingDown = false;
};

#endif // MUDLET_CREDENTIALMANAGER_H
