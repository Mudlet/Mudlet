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

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QPointer>
#include <functional>
#include <memory>
#include <vector>

class QTimer;

namespace QKeychain {
class Job;
class ReadPasswordJob;
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
    // Stalls the keychain behind a profile's own password lookup
    friend class TelnetLatePasswordTest;

public:
    explicit CredentialManager(QObject* parent = nullptr);
    ~CredentialManager();

    // Callback types for asynchronous operations
    using CredentialCallback = std::function<void(bool success, const QString& errorMessage)>;
    using CredentialRetrievalCallback = std::function<void(bool success, QString password, const QString& errorMessage)>;
    // timedOut: it was the lookup's deadline that answered, not the keychain
    using TimedRetrievalCallback = std::function<void(bool success, QString password, const QString& errorMessage, bool timedOut)>;
    using AvailabilityCallback = std::function<void(bool available, const QString& message)>;

    // Hybrid password management methods (preferred public API)
    // These methods intelligently choose between keychain and SecureStringUtils based on availability and portable mode
    void storePassword(const QString& profileName, const QString& key, const QString& password, CredentialCallback callback);
    void retrievePassword(const QString& profileName, const QString& key, CredentialRetrievalCallback callback);
    // For a caller that can still use a password the keychain hands over after the lookup gave up
    // on it, as it does when the user answers an access or unlock prompt late. callback is answered
    // exactly once, as above. When that answer is the deadline's, lateCallback is answered once
    // more with what the read the lookup was left waiting on finds, as long as lateContext still
    // exists by then. That read only: the places the lookup had not reached yet stay unread.
    void retrievePassword(const QString& profileName, const QString& key, TimedRetrievalCallback callback, QObject* lateContext, CredentialRetrievalCallback lateCallback);
    void removePassword(const QString& profileName, const QString& key, CredentialCallback callback);
    // Existence check that never hands the stored secret to the caller. QtKeychain has no metadata-only
    // lookup, so this reads the credential internally but forwards only whether one exists (scrubbing the
    // retrieved value), so callers such as UI code need not materialize the secret just to test presence.
    void credentialExists(const QString& profileName, const QString& key, std::function<void(bool exists)> callback);

    // What this manager's last store left readable by other accounts, or empty. Kept per manager, not
    // process-wide, so the report is about this store and not a failure elsewhere in the meantime.
    QString unprotectedSecretPath() const { return mUnprotectedSecretPath; }

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
    bool storeCredentialToFileForThisOperation(const QString& profileName, const QString& key, const QString& credential);
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
        TimedRetrievalCallback callback;
        CredentialRetrievalCallback lateCallback;
        QPointer<QObject> lateContext;
        std::vector<LookupStage> stages;
        // Parent of the lookup's deadline and of every read it starts, deleted once it has answered.
        QPointer<QObject> scope;
        // The read of the current stage, until it answers
        QPointer<QKeychain::ReadPasswordJob> currentRead;
        bool answered = false;
        std::size_t currentStage = 0;
        // The first read that failed for a reason other than there being no such entry, reported in
        // place of "not found" if nothing turns up.
        QString keychainError;
        // Whether any read has reached the store, answering either with a password or with "no such
        // entry". Until one has, a refusal is the store itself saying no - locked, or a prompt the
        // player dismissed - and the layouts behind it cannot be read either. Once one has, every
        // refusal after it is that entry's own, however many of them there are, and the chain runs
        // to the end: the password may be in a layout behind them.
        bool storeHasAnswered = false;
        // Set when the store has refused scmRefusalsBeforeGivingUpOnTheStore reads in a row without
        // answering any: every remaining keychain read would ask it the same question, and be
        // refused the same way, at the cost of another prompt. Only the file is read from then on.
        bool storeRefused = false;
        // Refusals since the last read the store answered. One can be an entry of its own that the
        // player - or a per-item ACL - has locked away while the rest of the store is readable, so
        // one is not enough to give up on the layouts behind it.
        int consecutiveRefusals = 0;
    };
    using LookupPtr = std::shared_ptr<Lookup>;

    void finishLookup(const LookupPtr& lookup, bool success, QString password, const QString& errorMessage, bool timedOut = false);
    void runLookupStage(const LookupPtr& lookup, std::size_t index);
    // When the store last refused a read before answering anything. Process-wide, because a caller
    // asking about two keys - the profile preferences ask about "reconnect" and then
    // "reconnect-token" - builds a CredentialManager for each, and the point is to spare the player
    // a second prompt for the answer the first one already gave. It runs out rather than latching,
    // so a player who unlocks their keychain is not left without it until they restart: while it is
    // open no read reaches the store, so the window's own expiry - or a lookup begun before it
    // opened, whose reads the store then answers - is what ends it. A write is deliberately not
    // enough, for the reason storeCredential() gives.
    static QElapsedTimer& storeRefusalTimer();
    // Forgets the refusal window. Process-wide state outlives one test, and a case that refuses a
    // read would otherwise decide what the cases after it are allowed to ask the store.
    static void forgetStoreRefusal();
    static bool storeRefusedRecently();
    static void noteStoreRefusal();
    static constexpr int scmStoreRefusalCooldownMs = 30000;
    // How many reads the store may refuse, without answering any of them, before a lookup stops
    // asking it. Two rather than one: a single refusal can be one entry's own, and an older layout
    // behind it may still hold the password, so the second read is what tells a locked or dismissed
    // store apart from an entry that is simply not readable.
    static constexpr int scmRefusalsBeforeGivingUpOnTheStore = 2;
    // Hands the answer of the read the deadline cut short to the lookup's lateCallback, whenever
    // the keychain gets round to giving it
    static void awaitLateAnswer(const LookupPtr& lookup);

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
    QTimer* mTimeoutTimer{nullptr};
    CredentialCallback mCurrentCallback;
    AvailabilityCallback mCurrentAvailabilityCallback;
    // What mCurrentJob is doing, for the log line if it is abandoned before it answers.
    QString mCurrentOperationDescription;
    QString mUnprotectedSecretPath;

    int mOperationTimeoutMs = OPERATION_TIMEOUT_MS;
    // Called with each keychain job just before it starts, so a test can make one stall or fail.
    // Returning false leaves the job unstarted for the hook to answer. A job that has reached the store
    // can't be cancelled and the store keeps a pointer to it, so only the store may answer that one.
    std::function<bool(QKeychain::Job*)> mJobStartHook;

    // Destruction flag to prevent operations during cleanup
    bool mShuttingDown = false;
};

#endif // MUDLET_CREDENTIALMANAGER_H
