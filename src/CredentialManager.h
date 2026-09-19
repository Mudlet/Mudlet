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

#include <QObject>
#include <QString>
#include <QPointer>
#include <functional>
#include <memory>
#include <vector>

class QTimer;

namespace QKeychain {
class Job;
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
    friend class CredentialManagerKeychainTest;

public:
    explicit CredentialManager(QObject* parent = nullptr);
    ~CredentialManager();

    // Callback types for asynchronous operations
    using CredentialCallback = std::function<void(bool success, const QString& errorMessage)>;
    using CredentialRetrievalCallback = std::function<void(bool success, QString password, const QString& errorMessage)>;
    using AvailabilityCallback = std::function<void(bool available, const QString& message)>;

    // Hybrid password management methods (preferred public API)
    // These methods intelligently choose between keychain and SecureStringUtils based on availability and portable mode
    void storePassword(const QString& profileName, const QString& key, const QString& password, CredentialCallback callback);
    void retrievePassword(const QString& profileName, const QString& key, CredentialRetrievalCallback callback);
    void removePassword(const QString& profileName, const QString& key, CredentialCallback callback);
    // Existence check that never hands the stored secret to the caller. QtKeychain has no metadata-only
    // lookup, so this reads the credential internally but forwards only whether one exists (scrubbing the
    // retrieved value), so callers such as UI code need not materialize the secret just to test presence.
    void credentialExists(const QString& profileName, const QString& key, std::function<void(bool exists)> callback);

    // Static fallback methods (for migration and test cleanup - uses encrypted file storage)
    static bool storeCredential(const QString& profileName, const QString& key, const QString& credential);
    static QString retrieveCredential(const QString& profileName, const QString& key);
    static bool removeCredential(const QString& profileName, const QString& key);

private:
    // Low-level async keychain methods (internal use only - use hybrid *Password methods instead)
    // profileName is used for file storage fallback (the service name is a hash that can't be reversed)
    void storeCredential(const QString& service, const QString& account, const QString& password, const QString& profileName, CredentialCallback callback);
    void removeCredential(const QString& service, const QString& account, const QString& profileName, CredentialCallback callback);
    // Combines the keychain delete outcome(s) with the file-fallback removal and reports the result
    void finishRemoveCredential(const QString& account, const QString& profileName, bool keychainSuccess, const QString& keychainError);

    // Check if QtKeychain is available and working (asynchronous)
    void isKeychainAvailable(AvailabilityCallback callback);

    // Password migration method - migrates plaintext passwords to encrypted storage
    void migratePassword(const QString& profileName, const QString& key, const QString& plaintextPassword, CredentialCallback callback);
    static constexpr int OPERATION_TIMEOUT_MS = 30000; // 30 seconds

    // Portable mode detection
    bool isPortableModeActive() const;
    bool shouldUseKeychain(const QString& profileName) const;

    void trackCurrentJob(QKeychain::Job* job);
    void startJob(QKeychain::Job* job);

    // Timeout and cleanup management
    void setupTimeout();
    void cleanupTimeout();
    void handleTimeout();
    void cleanupCurrentOperation();

    // Safety guard for keychain operation callbacks
    bool isOperationValid() const;

    // Static utility methods for fallback storage
    static QString generateFilePath(const QString& profileName, const QString& key);
    static QString generateLegacyFilePath(const QString& profileName, const QString& key);
    static QString readLegacyFileCredential(const QString& profileName, const QString& key);
    static QString ourLegacyFilePath(const QString& profileName, const QString& key);
    static void refreshLegacyFileCredential(const QString& profileName, const QString& key, const QString& credential);
    static void removeLegacyFileCredential(const QString& profileName, const QString& key);
    static QString generateServiceName(const QString& profileName, const QString& key);
    static QString generateLegacyServiceName(const QString& profileName, const QString& key);
    static bool isValidKeyName(const QString& key);
    static bool storeCredentialToFile(const QString& profileName, const QString& key, const QString& credential);
    static QString retrieveCredentialFromFile(const QString& profileName, const QString& key);
    static bool removeCredentialFromFile(const QString& profileName, const QString& key);

    // One place a lookup may find the password: a keychain entry, or the encrypted file. recover
    // re-files a password found there under the current name.
    struct LookupStage
    {
        QString description;
        QString service;
        QString key;
        bool fromFile = false;
        std::function<void(const QString& password)> recover;
    };

    // One retrievePassword() call on the keychain path, from its first read to its one answer. It
    // shares nothing with the single-operation state below, so lookups on one manager cannot disturb
    // each other or a store or removal running beside them.
    struct Lookup
    {
        QString profileName;
        QString key;
        CredentialRetrievalCallback callback;
        std::vector<LookupStage> stages;
        // Parent of the lookup's deadline and of every read it starts, deleted once it has answered.
        QPointer<QObject> scope;
        bool answered = false;
        std::size_t currentStage = 0;
        // The first read that failed for a reason other than there being no such entry, reported in
        // place of "not found" if nothing turns up.
        QString keychainError;
    };
    using LookupPtr = std::shared_ptr<Lookup>;

    void finishLookup(const LookupPtr& lookup, bool success, QString password, const QString& errorMessage);
    void runLookupStage(const LookupPtr& lookup, std::size_t index);

    // Each re-files a password a lookup recovered from an older format. Started just before the lookup
    // answers and independent of it, so a write that stalls cannot hold back the recovered password,
    // and deleting this manager from the caller's callback does not cut them short.
    void migrateCompatNamingEntry(const QString& service, const QString& password);
    void migrateOldFormatEntry(const QString& service, const QString& lookupService, const QString& account, const QString& password);
    // Re-files under the current name, then removes every colliding-format entry for the key.
    void migrateCollidingEntry(const QString& profileName, const QString& key, const QString& legacyService, const QString& password);
    void migrateLegacyEntry(const QString& profileName, const QString& key, const QString& password);
    static void deleteLegacyKeychainEntry(const QString& profileName, const std::function<bool(QKeychain::Job*)>& hook, int timeoutMs);

    // Current operation state
    QPointer<QKeychain::Job> mCurrentJob{nullptr};
    // Whether the keychain has answered mCurrentJob, so abandoning it can tell a job QtKeychain is
    // done with from one it still holds.
    bool mCurrentJobFinished = false;
    QTimer* mTimeoutTimer{nullptr};
    CredentialCallback mCurrentCallback;
    AvailabilityCallback mCurrentAvailabilityCallback;
    // What mCurrentJob is doing, for the log line if it is abandoned before it answers.
    QString mCurrentOperationDescription;

    int mOperationTimeoutMs = OPERATION_TIMEOUT_MS;
    // Called with each keychain job just before it starts, so a test can make one stall or fail.
    // Returning false leaves the job unstarted and hands it to the hook to answer: a credential
    // store call cannot be cancelled, so a job that has reached the store has to be answered - and
    // outlived - by the store alone, and a test that answers one itself would be deleting a job the
    // store still holds a pointer to.
    std::function<bool(QKeychain::Job*)> mJobStartHook;

    // Destruction flag to prevent operations during cleanup
    bool mShuttingDown = false;
};

#endif // MUDLET_CREDENTIALMANAGER_H
