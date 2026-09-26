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
#include "MudletApp.h"
#include "SecureStringUtils.h"
#include "utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSaveFile>
#include <QDataStream>
#include <QScopeGuard>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>

#include <memory>
#include <utility>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif

// Forward declaration to avoid including mudlet.h
class mudlet;

namespace {
// Whether a delete job's result means the credential is not left behind. EntryNotFound (nothing
// to delete) and NoBackendAvailable/NotImplemented (the keychain isn't the store here, so the
// file fallback is authoritative) are not failures; anything else is a genuine failure to remove
// an entry that may still exist.
bool keychainDeleteSucceeded(QKeychain::Error error)
{
    return error == QKeychain::NoError || error == QKeychain::EntryNotFound || error == QKeychain::NoBackendAvailable || error == QKeychain::NotImplemented;
}

// Worded so a caller can tell a refused argument from a storage failure
const auto scmInvalidKeyNameError = qsl("Key name is not valid for credential storage");

// QStandardPaths automatically handles portable mode configuration paths
QString credentialFilePath(const QString& profileComponent, const QString& keyComponent)
{
    return qsl("%1/profiles/%2/passwords/%3").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), profileComponent, keyComponent);
}

// Not tied to sanitizeForPath()'s limit, which may change: this records what is already on disk.
constexpr int scmLegacyMaxPathComponentLength = 50;

// sanitizeForPath() before it kept shortened names distinct, so credentials filed under the old
// name can still be found. The file counterpart of generateLegacyServiceName().
QString legacyPathComponent(const QString& input)
{
    static const auto unsafeChars = QRegularExpression(qsl(R"REGEX([/\\:*?"<>|])REGEX"));
    QString sanitized = input;
    sanitized.replace(unsafeChars, qsl("_"));
    return sanitized.left(scmLegacyMaxPathComponentLength);
}

// An older Mudlet writes only the legacy copy, so the copy written last holds the latest password.
// Timestamps the filesystem can't tell apart (whole-second FAT, network mounts) favour the current copy.
bool fileWrittenAfter(const QString& path, const QString& otherPath)
{
    const QFileInfo info(path);
    const QFileInfo otherInfo(otherPath);

    return info.exists() && (!otherInfo.exists() || info.lastModified() > otherInfo.lastModified());
}

bool writeCredentialFile(const QString& filePath, const QString& profileName, const QString& credential)
{
    const QString directoryPath = QFileInfo(filePath).absolutePath();

    if (!QDir().mkpath(directoryPath)) {
        qWarning() << "CredentialManager: Failed to create directory structure for" << filePath;
        return false;
    }

    // Holds only credentials, so owner-only too. The profile directory above is narrowed by
    // SecureStringUtils when the encryption key is written or read.
    SecureStringUtils::restrictDirectoryToOwner(directoryPath);

    // an empty credential is allowed - it stands for "no password"
    const QString encrypted = SecureStringUtils::encryptStringForProfile(credential, profileName);

    if (encrypted.isEmpty() && !credential.isEmpty()) {
        qWarning() << "CredentialManager: Failed to encrypt credential for profile" << profileName;
        return false;
    }

    const QByteArray payload = encrypted.toUtf8();
    // QSaveFile, not QFile: QFile empties the file on open, and a payload this small is only written in
    // close(), which returns void - so a full disk would leave an empty file and report success.
    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "CredentialManager: Failed to open file for writing:" << filePath << "Error:" << file.errorString();
        return false;
    }

    if (file.write(payload) != payload.size() || !file.commit()) {
        // whatever was there before is still there: QSaveFile discards the partial write
        qWarning() << "CredentialManager: Failed to write encrypted credential to file:" << filePath << "Error:" << file.errorString();
        return false;
    }

    SecureStringUtils::restrictFileToOwner(filePath);

    return true;
}
// Lets a keychain job outlive whoever started it, deleting itself once it finishes. QtKeychain's
// libsecret backend hands libsecret a raw pointer to a running job and cannot cancel the call, so
// deleting a job that is still waiting on the keychain reads freed memory when the keychain answers -
// after an unlock prompt left open, say. Until it does answer, the job holds QtKeychain's queue, which
// runs one job at a time for the whole process.
void detachJob(QKeychain::Job* job)
{
    job->setParent(nullptr);
    job->setAutoDelete(true);
}

// Whether a failed read says nothing is stored here, as opposed to the keychain refusing to answer.
// No backend at all is the former: the file fallback is then where a password would be.
bool readFoundNothing(QKeychain::Error error)
{
    return error == QKeychain::EntryNotFound || error == QKeychain::NoBackendAvailable || error == QKeychain::NotImplemented;
}

// Starts a job nobody waits on, detached from the start. It is never abandoned, for the reason
// detachJob() gives, so the one thing to do about a job that stops answering is to say so.
void startUnattendedJob(QKeychain::Job* job, const std::function<bool(QKeychain::Job*)>& hook, int timeoutMs, const QString& description)
{
    detachJob(job);
    // Before the watchdog: a job the hook takes over never reaches the keychain
    if (hook && !hook(job)) {
        return;
    }
    auto* watchdog = new QTimer(job);
    watchdog->setSingleShot(true);
    QObject::connect(watchdog, &QTimer::timeout, job, [description, timeoutMs]() {
        qWarning().noquote() << "CredentialManager: the" << description << "has had no answer from the keychain after" << timeoutMs << "ms, and every later keychain job waits behind it";
    });
    watchdog->start(timeoutMs);
    job->start();
}

} // namespace

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
    mCurrentAvailabilityCallback = nullptr;

    // Clean up operations - this is safe even during application shutdown
    cleanupCurrentOperation();

    // Lookups' reads are children of their scopes, which would take them down with this manager
    const auto jobs = findChildren<QKeychain::Job*>();
    for (auto* job : jobs) {
        detachJob(job);
    }
}

void CredentialManager::setupTimeout()
{
    cleanupTimeout(); // Clean up any existing timer

    mTimeoutTimer = new QTimer(this);
    mTimeoutTimer->setSingleShot(true);
    mTimeoutTimer->setInterval(mOperationTimeoutMs);

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
    auto availabilityCallback = std::exchange(mCurrentAvailabilityCallback, nullptr);

    // Clean up the timed-out operation before invoking the callback,
    // since the callback may start a new operation
    cleanupCurrentOperation();

    if (callback) {
        callback(false, qsl("Operation timed out"));
    } else if (availabilityCallback) {
        availabilityCallback(false, qsl("Operation timed out"));
    }
}

void CredentialManager::cleanupCurrentOperation()
{
    cleanupTimeout();

    if (mCurrentJob) {
        // Still set, so its answer was never handled: it is being superseded, has timed out, or is going
        // with this manager. Nothing is left to hear its result, so this is the only record of it.
        qWarning().noquote() << "CredentialManager: abandoned the" << mCurrentOperationDescription << "before the keychain answered, so its outcome is unknown";

        // If we're destroying or shutting down, avoid disconnect calls that might crash
        if (!mShuttingDown && !QCoreApplication::closingDown()) {
            // Only this manager's own connections. QtKeychain runs one job at a time for the whole
            // process and waits for the running one through its own connections to it, so dropping
            // those too would leave every later keychain job in the process queued for good.
            disconnect(mCurrentJob, nullptr, this, nullptr);
        }

        if (mCurrentJobFinished) {
            // The keychain has answered, so QtKeychain is done with the job and deleting it is both
            // safe and necessary: its queued result handler will not arrive, this manager may be going
            // away with it, and Job::emitFinished() read autoDelete as the job answered - too early for
            // detachJob() to make it delete itself now. Detached it would keep the password it carries.
            mCurrentJob->setParent(nullptr);
            mCurrentJob->deleteLater();
        } else {
            detachJob(mCurrentJob);
        }
        mCurrentJob = nullptr;
    }

    // Clear callbacks
    mCurrentCallback = nullptr;
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
    bool hasCallbacks = (mCurrentCallback || mCurrentAvailabilityCallback);

    if (!hasCallbacks) {
        qDebug() << "CredentialManager: Operation invalid - no callbacks set";
    }

    return hasCallbacks;
}

bool CredentialManager::isPortableModeActive() const
{
    // The settled answer, not a marker stat: a portable.txt naming a refused root leaves the marker while the
    // config root is the ordinary one, and treating that as portable would move credentials out of the keychain.
    return MudletApp::portableRootInUse();
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

// Narrowing failures are recorded process-wide, where another profile's credential work can add one
// mid-job. Clearing first leaves only this store's.
bool CredentialManager::storeCredentialToFileForThisOperation(const QString& profileName, const QString& key, const QString& credential)
{
    SecureStringUtils::takeUnprotectedSecretPath();
    const bool stored = storeCredentialToFile(profileName, key, credential);
    mUnprotectedSecretPath = SecureStringUtils::takeUnprotectedSecretPath();

    return stored;
}

void CredentialManager::storePassword(const QString& profileName, const QString& key, const QString& password, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, qsl("Profile name and key cannot be empty"));
        }

        return;
    }

    // Match the static API: a key it refuses would be filed where its migration and cleanup never look.
    if (!isValidKeyName(key)) {
        if (callback) {
            callback(false, scmInvalidKeyNameError);
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
        bool success = storeCredentialToFileForThisOperation(profileName, key, password);

        if (callback) {
            callback(success, success ? QString() : qsl("Failed to store password with SecureStringUtils"));
        }
    }
}

void CredentialManager::retrievePassword(const QString& profileName, const QString& key, CredentialRetrievalCallback callback)
{
    retrievePassword(
            profileName,
            key,
            [callback = std::move(callback)](bool success, QString password, const QString& errorMessage, bool) {
                if (callback) {
                    callback(success, std::move(password), errorMessage);
                } else {
                    SecureStringUtils::secureStringClear(password);
                }
            },
            nullptr,
            nullptr);
}

void CredentialManager::retrievePassword(const QString& profileName, const QString& key, TimedRetrievalCallback callback, QObject* lateContext, CredentialRetrievalCallback lateCallback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, QString(), qsl("Profile name and key cannot be empty"), false);
        }

        return;
    }

    // Match the static API: a key it refuses would be filed where its migration and cleanup never look.
    if (!isValidKeyName(key)) {
        if (callback) {
            callback(false, QString(), scmInvalidKeyNameError, false);
        }

        return;
    }

    // Safety check: Don't start new operations during shutdown
    if (QCoreApplication::closingDown()) {
        qWarning() << "CredentialManager: Rejecting retrievePassword operation during shutdown";

        if (callback) {
            callback(false, QString(), qsl("Application is shutting down"), false);
        }

        return;
    }

    if (shouldUseKeychain(profileName)) {
        auto lookup = std::make_shared<Lookup>();
        lookup->profileName = profileName;
        lookup->key = key;
        lookup->callback = std::move(callback);
        if (lateContext && lateCallback) {
            lookup->lateContext = lateContext;
            lookup->lateCallback = std::move(lateCallback);
        }
        lookup->scope = new QObject(this);

        const QString service = generateServiceName(profileName, key);
        const QString legacyService = generateLegacyServiceName(profileName, key);
#if defined(Q_OS_WIN)
        // Old-format entries live at TargetName == account; qtkeychain 0.17+ would look up
        // "account@service" and miss them, while an empty service resolves to the bare key on
        // every qtkeychain version (pre-0.17 ignores the service entirely)
        const QString oldFormatService;
#else
        const QString oldFormatService = service;
#endif

        // Every place the password may be, each read once and in this order, stopping at the first
        // that holds it. Whatever is recovered from an older layout is re-filed under the current
        // name, so once that succeeds the next lookup finds it sooner.
        auto& stages = lookup->stages;
        // Use service as the key - on Windows with qtkeychain before 0.17, only setKey() value is used as
        // the credential target, so using account ("character") would make all profiles share one
        stages.push_back({qsl("current format"), service, service, false, nullptr});
#if defined(Q_OS_WIN)
        // qtkeychain 0.17.0 changed the Windows Credential Manager TargetName from the bare key to
        // "key@service", so entries stored by builds linked against an older qtkeychain (which used
        // TargetName == key == service) are no longer found by the first read. A read with an empty
        // service resolves to TargetName == key on every qtkeychain version.
        stages.push_back({qsl("pre-0.17 qtkeychain naming"), QString(), service, false, [this, service](const QString& password) {
                              migrateCompatNamingEntry(service, password);
                          }});
#endif
        // Before the Windows keychain fix, credentials were stored with key=account, so on Windows
        // every profile shared one "character" entry
        stages.push_back({qsl("old key=account format"), oldFormatService, key, false, [this, service, oldFormatService, key](const QString& password) {
                              migrateOldFormatEntry(service, oldFormatService, key, password);
                          }});
        // The pre-4.20.0 keychain format, only ever used for these two keys
        if (!key.compare(qsl("password")) || !key.compare(qsl("character"))) {
            stages.push_back({qsl("pre-4.20.0 format"), qsl("Mudlet profile"), profileName, false, [this, profileName, key](const QString& password) {
                                  migrateLegacyEntry(profileName, key, password);
                              }});
        }
        stages.push_back({qsl("encrypted file"), QString(), QString(), true, nullptr});
        // The colliding format of 4.20.0 and 4.20.1, where profiles with similar names could share one
        // entry - last, so a password this profile holds anywhere else wins over one it may share.
        const auto recoverColliding = [this, profileName, key, legacyService](const QString& password) {
            migrateCollidingEntry(profileName, key, legacyService, password);
        };
        stages.push_back({qsl("colliding format"), legacyService, legacyService, false, recoverColliding});
#if defined(Q_OS_WIN)
        stages.push_back({qsl("colliding format under pre-0.17 qtkeychain naming"), QString(), legacyService, false, recoverColliding});
#else
        stages.push_back({qsl("colliding format with key=account"), legacyService, key, false, recoverColliding});
#endif

        // One deadline for the whole chain rather than one per read. The chain is several keychain
        // jobs deep and QtKeychain runs one job at a time for the whole process, so any read - or a
        // job elsewhere ahead of it in that queue - can stall the lookup, and dlgConnectionProfiles
        // holds its dialog until this answers.
        auto* deadline = new QTimer(lookup->scope);
        deadline->setSingleShot(true);
        connect(deadline, &QTimer::timeout, lookup->scope, [this, lookup]() {
            qWarning().noquote().nospace() << "CredentialManager: gave up looking up the saved password for profile \"" << lookup->profileName << "\", key \"" << lookup->key << "\" after "
                                           << mOperationTimeoutMs << "ms, waiting on the read of the " << lookup->stages[lookup->currentStage].description
                                           << " (or on another keychain job ahead of it in the queue)";
            // Before finishLookup() lets go of the read that is still out
            awaitLateAnswer(lookup);
            finishLookup(lookup, false, QString(), qsl("Operation timed out"), true);
        });
        deadline->start(mOperationTimeoutMs);

        runLookupStage(lookup, 0);
    } else {
        // Use SecureStringUtils directly (portable/test mode)
        QString password = retrieveCredentialFromFile(profileName, key);
        bool success = !password.isEmpty();

        if (callback) {
            // Empty password is normal for first-time profiles - not an error. Move the buffer so the
            // receiver takes sole ownership and can scrub the secret in place.
            callback(success, std::move(password), success ? QString() : qsl("No password stored in encrypted file storage"), false);
        }
    }
}

void CredentialManager::credentialExists(const QString& profileName, const QString& key, std::function<void(bool exists)> callback)
{
    // Take the password by value so this callback holds the sole owner of the secret buffer
    // (retrievePassword moves it in), then zero it in place before forwarding only whether a credential
    // exists. QString is copy-on-write, so scrubbing a shared copy would detach and leave the original
    // intact - sole ownership is what makes the wipe effective.
    retrievePassword(profileName, key, [callback](bool success, QString password, const QString&) {
        const bool exists = success && !password.isEmpty();
        SecureStringUtils::secureStringClear(password);
        if (callback) {
            callback(exists);
        }
    });
}

// Takes over a job's lifetime for one operation: its result handler deletes it, so QtKeychain must
// not, which leaves cleanupCurrentOperation() needing to know whether the keychain has answered yet.
void CredentialManager::trackCurrentJob(QKeychain::Job* job)
{
    job->setAutoDelete(false);
    mCurrentJob = job;
    mCurrentJobFinished = false;
    // Direct, unlike the result handlers, so this is set while the job is still emitting finished()
    connect(
            job,
            &QKeychain::Job::finished,
            this,
            [this]() {
                mCurrentJobFinished = true;
            },
            Qt::DirectConnection);
}

void CredentialManager::startJob(QKeychain::Job* job)
{
    if (mJobStartHook && !mJobStartHook(job)) {
        return;
    }
    job->start();
}

void CredentialManager::finishLookup(const LookupPtr& lookup, bool success, QString password, const QString& errorMessage, bool timedOut)
{
    if (lookup->answered) {
        SecureStringUtils::secureStringClear(password);
        return;
    }
    lookup->answered = true;
    if (lookup->scope) {
        // Any read still outstanding is detached rather than deleted with the scope; see detachJob().
        const auto jobs = lookup->scope->findChildren<QKeychain::Job*>(Qt::FindDirectChildrenOnly);
        for (auto* job : jobs) {
            detachJob(job);
        }
        lookup->scope->deleteLater();
    }
    auto callback = std::exchange(lookup->callback, nullptr);
    if (callback) {
        // Move so ownership of the secret buffer threads through to the final callback.
        callback(success, std::move(password), errorMessage, timedOut);
    } else {
        SecureStringUtils::secureStringClear(password);
    }
}

void CredentialManager::awaitLateAnswer(const LookupPtr& lookup)
{
    QKeychain::ReadPasswordJob* read = lookup->currentRead;
    if (lookup->answered || !lookup->lateCallback || !read) {
        return;
    }

    // The read is the context: finishLookup() detaches it, so it outlives the lookup and this
    // manager and deletes itself once the keychain answers, taking this handler with it. A
    // password found in an older layout is handed over without being re-filed, which the next
    // lookup does.
    connect(read,
            &QKeychain::Job::finished,
            read,
            [read, lateContext = lookup->lateContext, lateCallback = lookup->lateCallback, profileName = lookup->profileName, description = lookup->stages[lookup->currentStage].description]() {
                const QKeychain::Error error = read->error();
                QString password = (error == QKeychain::NoError) ? read->textData() : QString();
                if (!lateContext) {
                    qWarning().noquote() << "CredentialManager: the keychain answered the read of the" << description << "for profile" << profileName
                                         << "after its lookup had timed out, but whatever asked for it has gone";
                    SecureStringUtils::secureStringClear(password);
                    return;
                }
                if (password.isEmpty()) {
                    // Told apart from a prompt that never gets an answer, which logs nothing further
                    qWarning().noquote() << "CredentialManager: the keychain answered the read of the" << description << "for profile" << profileName
                                         << "after its lookup had timed out, without the password -" << read->errorString();
                    // That read alone: the places the lookup had not reached are still unread
                    const QString errorMessage = (error == QKeychain::NoError || readFoundNothing(error)) ? qsl("No password in the %1 for profile %2").arg(description, profileName)
                                                                                                          : qsl("Could not read the keychain: %1").arg(read->errorString());
                    lateCallback(false, QString(), errorMessage);
                    return;
                }
                qDebug().noquote() << "CredentialManager: the keychain answered the read of the" << description << "for profile" << profileName << "after its lookup had timed out, with the password";
                lateCallback(true, std::move(password), QString());
            });
}

void CredentialManager::runLookupStage(const LookupPtr& lookup, std::size_t index)
{
    if (lookup->answered || !lookup->scope) {
        return;
    }
    if (index >= lookup->stages.size()) {
        // A keychain that refused a read may still hold the password, so say so rather than
        // reporting that nothing is stored.
        const QString error =
                lookup->keychainError.isEmpty() ? qsl("No stored credentials found for profile %1").arg(lookup->profileName) : qsl("Could not read the keychain: %1").arg(lookup->keychainError);
        finishLookup(lookup, false, QString(), error);
        return;
    }

    lookup->currentStage = index;
    const LookupStage& stage = lookup->stages[index];
    if (stage.fromFile) {
        QString password = retrieveCredentialFromFile(lookup->profileName, lookup->key);
        if (password.isEmpty()) {
            runLookupStage(lookup, index + 1);
            return;
        }
        finishLookup(lookup, true, std::move(password), QString());
        return;
    }

    auto* job = new QKeychain::ReadPasswordJob(stage.service, lookup->scope);
    job->setKey(stage.key);
    job->setAutoDelete(false);
    lookup->currentRead = job;
    connect(job, &QKeychain::Job::finished, lookup->scope, [this, lookup, job, index]() {
        lookup->currentRead = nullptr;
        // Not before the lookup has moved on: the job is still emitting finished(), and a caller's
        // callback can flush deferred deletes - loading a profile does - which would free it
        // underneath QtKeychain.
        const auto deleteJob = qScopeGuard([job]() {
            job->deleteLater();
        });
        const QKeychain::Error error = job->error();
        QString password = (error == QKeychain::NoError) ? job->textData() : QString();
        const QString errorString = job->errorString();
        if (lookup->answered) {
            SecureStringUtils::secureStringClear(password);
            return;
        }

        const LookupStage& finishedStage = lookup->stages[index];
        if (error == QKeychain::NoError && !password.isEmpty()) {
            qDebug() << "CredentialManager: Found the password for profile" << lookup->profileName << "in the" << finishedStage.description;
            if (finishedStage.recover && lookup->keychainError.isEmpty()) {
                finishedStage.recover(password);
            } else if (finishedStage.recover) {
                // A place read earlier refused to answer and may hold a newer password, which re-filing
                // this one under the current name could overwrite
                qWarning() << "CredentialManager: Not re-filing the password found in the" << finishedStage.description << "for profile" << lookup->profileName
                           << "under the current name, as an earlier keychain read failed";
            }
            finishLookup(lookup, true, std::move(password), QString());
            return;
        }
        if (error != QKeychain::NoError && !readFoundNothing(error)) {
            // A hard keychain error is distinct from "no such entry" and is the likely reason a saved
            // password appears to have vanished - surface it rather than treating it as not found
            qWarning() << "CredentialManager: Keychain read of the" << finishedStage.description << "failed for profile" << lookup->profileName << "-" << errorString;
            if (lookup->keychainError.isEmpty()) {
                lookup->keychainError = errorString;
            }
        }
        runLookupStage(lookup, index + 1);
    });
    startJob(job);
}

void CredentialManager::migrateCompatNamingEntry(const QString& service, const QString& password)
{
    // Re-store through a normal write (key == service) so the entry lands under the
    // naming scheme of the linked qtkeychain version
    auto* migrateJob = new QKeychain::WritePasswordJob(service);
    migrateJob->setKey(service);
    migrateJob->setTextData(password);

    connect(migrateJob, &QKeychain::WritePasswordJob::finished, migrateJob, [migrateJob, service, hook = mJobStartHook, timeoutMs = mOperationTimeoutMs]() {
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
                connect(cleanupJob, &QKeychain::DeletePasswordJob::finished, cleanupJob, [cleanupJob, service]() {
                    if (cleanupJob->error() == QKeychain::NoError || cleanupJob->error() == QKeychain::EntryNotFound) {
                        qDebug() << "CredentialManager: Pre-0.17 entry cleaned up for service:" << service;
                    } else {
                        qWarning() << "CredentialManager: Failed to clean up pre-0.17 entry for service:" << service << "-" << cleanupJob->errorString();
                    }
                });
                startUnattendedJob(cleanupJob, hook, timeoutMs, qsl("removal of the pre-0.17 entry %1").arg(service));
            }
#endif
        } else {
            // The password was recovered and returned, but persisting it under the current scheme
            // failed - the primary read will keep missing, so this path re-runs on every launch until
            // the write succeeds
            qWarning() << "CredentialManager: Recovered the password but failed to persist it under the current naming scheme (will retry next read):" << migrateJob->errorString();
        }
    });

    startUnattendedJob(migrateJob, mJobStartHook, mOperationTimeoutMs, qsl("re-filing of a password found under pre-0.17 naming as %1").arg(service));
}

void CredentialManager::migrateOldFormatEntry(const QString& service, const QString& lookupService, const QString& account, const QString& password)
{
    // Store in new format (key=service) for future use
    auto* migrateJob = new QKeychain::WritePasswordJob(service);
    migrateJob->setKey(service);
    migrateJob->setTextData(password);

    connect(migrateJob, &QKeychain::WritePasswordJob::finished, migrateJob, [migrateJob, service, lookupService, account, hook = mJobStartHook, timeoutMs = mOperationTimeoutMs]() {
        if (migrateJob->error() == QKeychain::NoError) {
            qDebug() << "CredentialManager: Migration to new format successful, cleaning up old entry";

            auto* cleanupJob = new QKeychain::DeletePasswordJob(lookupService);
            cleanupJob->setKey(account); // Old format key
            connect(cleanupJob, &QKeychain::DeletePasswordJob::finished, cleanupJob, [cleanupJob, service]() {
                if (cleanupJob->error() == QKeychain::NoError || cleanupJob->error() == QKeychain::EntryNotFound) {
                    qDebug() << "CredentialManager: Old format entry cleaned up for service:" << service;
                } else {
                    qWarning() << "CredentialManager: Failed to clean up old format entry for service:" << service << "-" << cleanupJob->errorString();
                }
            });
            startUnattendedJob(cleanupJob, hook, timeoutMs, qsl("removal of the old key=account entry for %1").arg(service));
        } else {
            // The password was recovered and returned, but persisting it under the new format failed -
            // the primary read will keep missing, so this path re-runs on every launch until the write
            // succeeds
            qWarning() << "CredentialManager: Recovered the password but failed to persist it under the new format (will retry next read):" << migrateJob->errorString();
        }
    });

    startUnattendedJob(migrateJob, mJobStartHook, mOperationTimeoutMs, qsl("re-filing of a password found in the old key=account format as %1").arg(service));
}

void CredentialManager::migrateCollidingEntry(const QString& profileName, const QString& key, const QString& legacyService, const QString& password)
{
    qDebug() << "CredentialManager: Migrating password from colliding format for" << profileName;

    // Parentless, and deleted once its store answers: a caller commonly deletes this manager from the
    // callback the recovered password reaches, which would otherwise cut the migration short.
    auto* migrator = new CredentialManager();
    migrator->mJobStartHook = mJobStartHook;
    migrator->mOperationTimeoutMs = mOperationTimeoutMs;
    migrator->storePassword(profileName, key, password, [migrator, legacyService, key, hook = mJobStartHook, timeoutMs = mOperationTimeoutMs](bool migrationSuccess, const QString& migrationError) {
        migrator->deleteLater();
        if (!migrationSuccess) {
            qWarning() << "CredentialManager: Migration failed:" << migrationError;
            return;
        }
        // Only clean up old colliding entry if version > 4.20.1
        // The colliding format bug existed in 4.20.0 and 4.20.1
        // Preserving the entry allows users to switch back to those versions
#ifdef APP_VERSION
        const QString currentVersion = QString(APP_VERSION);
        QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
        const QVersionNumber collidingFormatVersion = QVersionNumber(4, 20, 1);

        // Dev/test/PTB builds represent the "next release", so bump version for comparison
        const QString buildSuffix = MudletApp::buildSuffix();
        if (buildSuffix.startsWith(qsl("-dev")) || buildSuffix.startsWith(qsl("-test")) || buildSuffix.startsWith(qsl("-ptb"))) {
            appVersion = QVersionNumber(appVersion.majorVersion(), appVersion.minorVersion(), appVersion.microVersion() + 1);
        }

        if (appVersion <= collidingFormatVersion) {
            return;
        }

        // Deletes the keychain entries directly: removeCredential() also deletes the encrypted file,
        // which is where storePassword() put the password if the keychain refused it. Every layout
        // the colliding format was written in goes, whichever of them this was recovered from.
        QList<QPair<QString, QString>> sweptEntries{{legacyService, legacyService}};
#if defined(Q_OS_WIN)
        Q_UNUSED(key)
        // A pre-0.17 bare entry (TargetName == legacy service)
        sweptEntries.append({QString(), legacyService});
#else
        sweptEntries.append({legacyService, key});
#endif
        for (const auto& [sweptService, sweptKey] : std::as_const(sweptEntries)) {
            auto* sweepJob = new QKeychain::DeletePasswordJob(sweptService);
            sweepJob->setKey(sweptKey);
            connect(sweepJob, &QKeychain::DeletePasswordJob::finished, sweepJob, [sweepJob, legacyService]() {
                if (sweepJob->error() != QKeychain::NoError && sweepJob->error() != QKeychain::EntryNotFound) {
                    qWarning() << "CredentialManager: Failed to clean up colliding entry" << legacyService << "-" << sweepJob->errorString();
                }
            });
            startUnattendedJob(sweepJob, hook, timeoutMs, qsl("removal of the colliding entry %1").arg(legacyService));
        }
#endif
    });
}

void CredentialManager::migrateLegacyEntry(const QString& profileName, const QString& key, const QString& password)
{
    // Parentless for the same reason as migrateCollidingEntry()
    auto* migrator = new CredentialManager();
    migrator->mJobStartHook = mJobStartHook;
    migrator->mOperationTimeoutMs = mOperationTimeoutMs;
    migrator->storePassword(profileName, key, password, [migrator, profileName, hook = mJobStartHook, timeoutMs = mOperationTimeoutMs](bool migrationSuccess, const QString& migrationError) {
        migrator->deleteLater();
        if (!migrationSuccess) {
            qWarning() << "CredentialManager: Migration failed:" << migrationError;
            return;
        }
        // Only clean up legacy entry after successful migration if this version is >= 4.20.0
        // This prevents breaking compatibility with older Mudlet versions
#ifdef APP_VERSION
        const QString currentVersion = QString(APP_VERSION);
        const QVersionNumber appVersion = QVersionNumber::fromString(currentVersion);
        const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);

        if (appVersion >= secureStorageVersion) {
            deleteLegacyKeychainEntry(profileName, hook, timeoutMs);
        }
#endif
    });
}

void CredentialManager::removePassword(const QString& profileName, const QString& key, CredentialCallback callback)
{
    if (profileName.isEmpty() || key.isEmpty()) {
        if (callback) {
            callback(false, qsl("Profile name and key cannot be empty"));
        }
        return;
    }

    // Match the static API: a key it refuses would be filed where its migration and cleanup never look.
    if (!isValidKeyName(key)) {
        if (callback) {
            callback(false, scmInvalidKeyNameError);
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
            removeCredential(legacyService, key, profileName, [profileName, key, callback, keychainSuccess, keychainError](bool oldSuccess, const QString& oldError) {
                // The credential must be gone from the current-format entry, the legacy colliding
                // entry, and the file fallback; a failure to remove any of them is a real failure
                // rather than something the others can paper over
                if (callback) {
                    if (keychainSuccess && oldSuccess) {
                        callback(true, QString());
                    } else {
                        callback(false, qsl("Failed to remove from keychain: %1").arg(!keychainSuccess ? keychainError : oldError));
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
    mUnprotectedSecretPath.clear();

    auto* writeJob = new QKeychain::WritePasswordJob(service, this);
    // Use service as the key - on Windows, only setKey() value is used as the credential target,
    // so using account ("character") would cause all profiles to share the same credential
    writeJob->setKey(service);

    // Store password directly in keychain (keychain handles encryption)
    writeJob->setTextData(password);

    trackCurrentJob(writeJob);
    mCurrentCallback = callback;
    mCurrentOperationDescription = qsl("keychain write for profile \"%1\", key \"%2\" (entry \"%3\")").arg(profileName, account, service);

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

                    bool fileSuccess = storeCredentialToFileForThisOperation(profileName, account, password);

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

    startJob(writeJob);
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

    trackCurrentJob(deleteJob);
    mCurrentCallback = callback;
    mCurrentOperationDescription = qsl("keychain removal for profile \"%1\", key \"%2\" (entry \"%3\")").arg(profileName, account, service);

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

                const bool primarySuccess = keychainDeleteSucceeded(deleteJob->error());
                const QString primaryError = primarySuccess ? QString() : deleteJob->errorString();
                deleteJob->deleteLater();

#if defined(Q_OS_WIN)
                // A pre-0.17 bare entry (TargetName == service) may still exist if it was written by
                // a build linked against an older qtkeychain and no read has migrated it yet; on
                // 0.17+ the delete above resolved to "service@service" and left that copy behind,
                // where the compat migration would resurrect it on a later read. Sweep the bare name
                // too, and await it so a genuine failure to remove it is reported rather than
                // silently dropped. On pre-0.17 both deletes resolve to the same TargetName, so this
                // sweep is a harmless EntryNotFound no-op.
                mCurrentJob = nullptr;
                auto* bareJob = new QKeychain::DeletePasswordJob(QString(), this);
                bareJob->setKey(service);
                trackCurrentJob(bareJob);
                // Abandoning this one can leave a pre-0.17 bare entry behind, and a later read's compat
                // migration would restore the credential from it.
                mCurrentOperationDescription = qsl("bare-name keychain sweep for profile \"%1\", key \"%2\"").arg(profileName, account);
                setupTimeout();

                connect(
                        bareJob,
                        &QKeychain::DeletePasswordJob::finished,
                        this,
                        [this, bareJob, service, account, profileName, primarySuccess, primaryError]() {
                            if (!isOperationValid()) {
                                qWarning() << "CredentialManager: Ignoring keychain callback - operation no longer valid";
                                bareJob->deleteLater();
                                return;
                            }

                            cleanupTimeout();

                            const bool bareSuccess = keychainDeleteSucceeded(bareJob->error());
                            const QString bareError = bareSuccess ? QString() : bareJob->errorString();
                            bareJob->deleteLater();

                            const bool keychainSuccess = primarySuccess && bareSuccess;
                            const QString keychainError = !primarySuccess ? primaryError : bareError;
                            finishRemoveCredential(account, profileName, keychainSuccess, keychainError);
                        },
                        Qt::QueuedConnection);

                startJob(bareJob);
#else
                finishRemoveCredential(account, profileName, primarySuccess, primaryError);
#endif
            },
            Qt::QueuedConnection); // Use queued connection for additional safety

    startJob(deleteJob);
}

void CredentialManager::finishRemoveCredential(const QString& account, const QString& profileName, bool keychainSuccess, const QString& keychainError)
{
    const bool fileSuccess = removeCredentialFromFile(profileName, account);

    // A removal only succeeds if the credential is gone from every store it could be in: a hard
    // keychain error means an entry may still be readable, so it must not be masked by the file
    // fallback reporting success simply because no file copy existed.
    const bool success = keychainSuccess && fileSuccess;
    QString errorMessage;

    if (!success) {
        if (!keychainSuccess) {
            errorMessage = qsl("Failed to remove from keychain: %1").arg(keychainError);
        } else {
            errorMessage = qsl("Failed to remove encrypted credential file for profile %1").arg(profileName);
        }
    }

    if (mCurrentCallback && isOperationValid()) {
        auto callback = mCurrentCallback; // Copy callback to avoid use-after-free
        mCurrentCallback = nullptr;
        mCurrentJob = nullptr;

        callback(success, errorMessage);
    }
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

    trackCurrentJob(testJob);
    mCurrentAvailabilityCallback = callback;
    mCurrentOperationDescription = qsl("keychain availability probe");

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

    startJob(testJob);
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

    if (!writeCredentialFile(filePath, profileName, credential)) {
        return false;
    }

    // Only after the current file is committed: they live in separate directories, so one can fail alone,
    // and a failed store must not leave old and new Mudlets with different passwords. If this leaves the
    // legacy file newer, the next read copies it across.
    refreshLegacyFileCredential(profileName, key, credential);

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

    // An older Mudlet uses only the truncated path, so a newer copy there is the latest password
    const QString legacyPath = generateLegacyFilePath(profileName, key);

    if (!legacyPath.isEmpty() && fileWrittenAfter(legacyPath, filePath)) {
        const QString migrated = readLegacyFileCredential(profileName, key);

        if (!migrated.isEmpty()) {
            // Copied, not moved: an older Mudlet sharing this config directory looks only there
            if (!writeCredentialFile(filePath, profileName, migrated)) {
                qWarning() << "CredentialManager: could not copy the newer credential at" << legacyPath << "across to" << filePath << "- it will be read from the older path again next time";
            }

            qDebug() << "CredentialManager: Found the" << key << "credential for profile" << profileName << "in the encrypted file left by the earlier naming scheme";

            return migrated;
        }

        // Undecryptable: a profile this one used to collide with wrote it, or it is damaged
        if (QFile::exists(filePath)) {
            qWarning() << "CredentialManager: the credential file" << legacyPath << "is newer than" << filePath << "but holds nothing profile" << profileName
                       << "can decrypt - the older copy is being used instead";
        }
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Only log warning if file should exist (not for first-time access)
        if (file.exists()) {
            qWarning() << "CredentialManager: Failed to open existing file for reading:" << filePath << "Error:" << file.errorString();
        }

        return QString();
    }

    // Credentials from before this narrowing, or from an older Mudlet (which uses the umask), may be
    // world-readable.
    SecureStringUtils::restrictFileToOwner(filePath);
    SecureStringUtils::restrictDirectoryToOwner(QFileInfo(filePath).absolutePath());

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
    } else {
        // Logged here because the caller can't tell keychain from file fallback. The key is named because
        // this also serves the proxy password and credentialExists()'s reconnect token.
        qDebug() << "CredentialManager: Found the" << key << "credential for profile" << profileName << "in the encrypted file";
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

    // The legacy file too, or the next read would copy it back and resurrect the password
    removeLegacyFileCredential(profileName, key);

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

    return credentialFilePath(MudletApp::sanitizeForPath(profileName), MudletApp::sanitizeForPath(key));
}

// Path under the old truncate-to-50 scheme, or empty when it can't safely be claimed: it is the current
// path anyway, or the key is long enough that another key's credential may be there.
QString CredentialManager::generateLegacyFilePath(const QString& profileName, const QString& key)
{
    const QString currentPath = generateFilePath(profileName, key);

    if (currentPath.isEmpty()) {
        return QString();
    }

    // Keys this long share one legacy file and the profile's encryption key, so decrypting can't tell which
    // key wrote it; leave it and have the password re-entered. A key of exactly the cap was never truncated
    // but still lands on the path longer keys were cut to, hence >=.
    if (key.length() >= scmLegacyMaxPathComponentLength) {
        return QString();
    }

    const QString legacyPath = credentialFilePath(legacyPathComponent(profileName), legacyPathComponent(key));
    return legacyPath == currentPath ? QString() : legacyPath;
}

// Empty when nothing there is this profile's. Decrypts rather than just reading: a file this profile
// can't decrypt belongs to a profile it used to collide with.
QString CredentialManager::readLegacyFileCredential(const QString& profileName, const QString& key)
{
    const QString legacyPath = generateLegacyFilePath(profileName, key);

    if (legacyPath.isEmpty()) {
        return QString();
    }

    QFile file(legacyPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (file.exists()) {
            qWarning() << "CredentialManager: Failed to open credential file left by the earlier naming scheme:" << legacyPath << "Error:" << file.errorString();
        }

        return QString();
    }

    // Legacy copies were written by older Mudlets, so are the likeliest to be world-readable
    SecureStringUtils::restrictFileToOwner(legacyPath);
    SecureStringUtils::restrictDirectoryToOwner(QFileInfo(legacyPath).absolutePath());

    const QString encrypted = QString::fromUtf8(file.readAll());
    file.close();

    return SecureStringUtils::decryptStringForProfile(encrypted, profileName);
}

// Only decrypting tells our legacy file from one a formerly-colliding profile left, so refresh and
// remove must check through this.
QString CredentialManager::ourLegacyFilePath(const QString& profileName, const QString& key)
{
    QString existing = readLegacyFileCredential(profileName, key);
    const bool ours = !existing.isEmpty();
    SecureStringUtils::secureStringClear(existing);

    return ours ? generateLegacyFilePath(profileName, key) : QString();
}

void CredentialManager::removeLegacyFileCredential(const QString& profileName, const QString& key)
{
    const QString legacyPath = ourLegacyFilePath(profileName, key);

    if (!legacyPath.isEmpty() && !QFile::remove(legacyPath)) {
        qWarning() << "CredentialManager: Failed to remove credential file left by the earlier naming scheme:" << legacyPath;
    }
}

// Keeps the legacy file in step so an older Mudlet doesn't offer a stale password. Never creates it:
// two profiles sharing their first 50 characters would then share it.
void CredentialManager::refreshLegacyFileCredential(const QString& profileName, const QString& key, const QString& credential)
{
    const QString legacyPath = ourLegacyFilePath(profileName, key);

    if (legacyPath.isEmpty()) {
        return;
    }

    // Written empty, the file could no longer be proven ours, so never refreshed or removed again
    if (credential.isEmpty()) {
        if (!QFile::remove(legacyPath)) {
            qWarning() << "CredentialManager: Failed to remove credential file left by the earlier naming scheme:" << legacyPath;
        }

        return;
    }

    if (!writeCredentialFile(legacyPath, profileName, credential)) {
        qWarning() << "CredentialManager: the copy of this profile's credential that an older Mudlet reads," << legacyPath
                   << "could not be brought up to date - that Mudlet will go on offering the previous password for profile" << profileName;
    }
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

void CredentialManager::deleteLegacyKeychainEntry(const QString& profileName, const std::function<bool(QKeychain::Job*)>& hook, int timeoutMs)
{
    if (profileName.isEmpty()) {
        return;
    }

    // Legacy format used service="Mudlet profile" and key=profileName
    const QString legacyService = qsl("Mudlet profile");

    auto* deleteJob = new QKeychain::DeletePasswordJob(legacyService);
    deleteJob->setKey(profileName);

    connect(deleteJob, &QKeychain::DeletePasswordJob::finished, deleteJob, [deleteJob, profileName]() {
        if (deleteJob->error() == QKeychain::NoError) {
            qDebug() << "CredentialManager: Deleted legacy entry for profile" << profileName;
        } else {
            qDebug() << "CredentialManager: Failed to delete legacy entry for profile" << profileName << ":" << deleteJob->errorString();
        }
    });

    startUnattendedJob(deleteJob, hook, timeoutMs, qsl("removal of the pre-4.20.0 entry for profile %1").arg(profileName));
}
