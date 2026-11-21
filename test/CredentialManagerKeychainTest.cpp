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

#include "GroupedTest.h"
#include <CredentialManager.h>
#include <MudletApp.h>
#include <SecureStringUtils.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QSettings>
#include <QtTest/QtTest>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUuid>
#include <QVersionNumber>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif

#include <memory>
#if !defined(Q_OS_WIN)
#include <unistd.h>
#endif

using namespace std::chrono_literals;

// Exercises CredentialManager's real keychain paths against the live platform credential
// store, in particular the migrations for the qtkeychain 0.17.0 Windows naming change
// (TargetName moved from the bare key to "key@service"). Unlike CredentialManagerTest,
// this deliberately does NOT set MUDLET_TEST_MODE, which would force file storage. It is
// kept as a separate executable so these environment-dependent tests cannot affect that
// hermetic unit suite, and so ctest can rerun or exclude them independently.
//
// The historical entry layouts are simulated with raw QKeychain jobs using an empty
// service, which resolves to TargetName == key on every qtkeychain version - the same
// primitive the migration code itself relies on. That means the pre-0.17, old-format and
// colliding-format layouts can all be planted and verified regardless of which qtkeychain
// is linked, and the expected outcomes branch on QTKEYCHAIN_LINKED_VERSION.
//
// The Windows naming migrations skip elsewhere (the bodies still compile on all platforms). The
// lookup-chain tests run on every platform, with a JobStaller standing in for the store on the jobs
// they care about; the three that plant entries under their real service skip when no credential
// store is available.

class CredentialManagerKeychainTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void testRoundTrip();
    void testBareEntryMigration();
    void testRemoveSweepsBareEntry();
    void testOldFormatMigration();
    void testCollidingFormatRecovery();
    void testALookupAnswersWhenItsFirstReadStalls();
    void testALookupWhoseKeychainStallsStillReadsTheEncryptedFile();
    void testALookupAnswersWhicheverLaterReadStalls_data();
    void testALookupAnswersWhicheverLaterReadStalls();
    void testALookupReadsEachPlaceOnceInOrder_data();
    void testALookupReadsEachPlaceOnceInOrder();
    void testATimedOutRemovalDoesNotStopLaterKeychainJobs();
    void testAStoreStartedFromATimedOutCallbackIsNotTornDownWithIt();
    void testALookupIsNotDisturbedByAnotherOnTheSameManager();
    void testDeletingAManagerMidLookupLeavesItsReadToFinish();
    void testAWriteTheKeychainHasAnsweredIsNotOrphanedWithItsPassword();
    void testACallbackThatFlushesDeferredDeletesDoesNotDeleteTheAnsweringRead();
    void testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat_data();
    void testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat();
    void testARecoveredPasswordIsReturnedWhileItsMigrationStalls();
    void testARecoveredPasswordSurvivesAKeychainThatRefusesToStoreIt();
    void testAStoreTheKeychainRefusesSaysItWentToTheFile();
    void testAKeychainErrorIsReportedRatherThanNothingFound_data();
    void testAKeychainErrorIsReportedRatherThanNothingFound();
    void testAnUndecryptableFileIsReportedRatherThanNothingFound_data();
    void testAnUndecryptableFileIsReportedRatherThanNothingFound();
    void testAnUndecryptableFileIsReportedWhileTheKeychainStillRefuses();
    void testAnUndecryptableFileIsReportedWhenTheKeychainStalls_data();
    void testAnUndecryptableFileIsReportedWhenTheKeychainStalls();
    void testAStoreThatRefusesIsNotAskedForEveryOtherLayout();
    void testAFreshRefusalSparesTheNextLookupTheStore();
    void testARefusalOfTheSignInsOnlyKeychainReadSparesTheNextLookup();
    void testTheProfileStoragePreferenceKeepsTheKeychainOutOfIt();
    void testTheMudletAppPreferenceKeepsTheKeychainOutOfIt();
    void testARemovalTheClientMakesItselfLeavesTheKeychainAlone();
    void testTheForgetCheckSeesAKeychainCopyTheProfileLacks();
    void testARefusalForOneProfileDoesNotSkipTheStoreForAnother();
    void testATrimmedLookupStillFindsTheProfilesOwnCopy();
    void testForgettingSaysSoWhenTheKeychainCopyRemains();
    void testAKeychainNotOnDBusHoldsNothingToRemove();
    void testRefusalsAfterTheStoreHasAnsweredDoNotStopTheChain();
    void testOneUnreadableEntryDoesNotHideAnOlderLayout();
    void testASignInKeyIsNotLookedForInLayoutsOlderThanItself();
    void testALookupThatAnswersInTimeOwesNoLateAnswer();
    void testALookupThatFindsNothingSaysWhetherTheStoreAnswered_data();
    void testALookupThatFindsNothingSaysWhetherTheStoreAnswered();
    void testALateAnswerFollowsALookupThatTimedOut_data();
    void testALateAnswerFollowsALookupThatTimedOut();
    void testALateAnswerIsDroppedOnceWhatAskedForItHasGone();
    void testALateAnswerHandsOverThePassword();
    void testALateAnswerFollowsTheFilesOlderCopy();
    void testAChosenRootsKeychainEntriesAreItsOwn();
    void testAChosenRootsEntriesAreNamedForTheFolderNotItsSpelling();
    void testAChosenRootReadsTheSaveItsKeychainRefusedFirst();
    void testAnUnreadableFileUnderAChosenRootIsNotPassedOverForTheKeychain_data();
    void testAnUnreadableFileUnderAChosenRootIsNotPassedOverForTheKeychain();
    void testUnderAChosenRootAKeychainStoreThatWillNotOpenIsNotTakenForAnEmptyOne_data();
    void testUnderAChosenRootAKeychainStoreThatWillNotOpenIsNotTakenForAnEmptyOne();
    void testOnlyAnOlderNamedCopyTheProfileDecryptsAnswersFirstUnderAChosenRoot_data();
    void testOnlyAnOlderNamedCopyTheProfileDecryptsAnswersFirstUnderAChosenRoot();
    void testUnderAChosenRootTheFileCopyGoesOnlyOnceTheKeychainGivesTheSaveBack_data();
    void testUnderAChosenRootTheFileCopyGoesOnlyOnceTheKeychainGivesTheSaveBack();
    void testUnderAChosenRootNeitherASaveNorAForgetSucceedsWhileACopyIsStillRead_data();
    void testUnderAChosenRootNeitherASaveNorAForgetSucceedsWhileACopyIsStillRead();
    void testUnderAChosenRootAnAnswerAfterTheTimeoutLeavesTheFileCopy_data();
    void testUnderAChosenRootAnAnswerAfterTheTimeoutLeavesTheFileCopy();
    void testTheForgetCheckUnderAChosenRootAsksTheKeychainOnlyWhenItServesIt_data();
    void testTheForgetCheckUnderAChosenRootAsksTheKeychainOnlyWhenItServesIt();

private:
    QTemporaryDir mConfigDir;
    QString mProfile;
    QString mKey;
    bool mStoreAvailable = false;
};

namespace {

constexpr int kWaitMs = 15000;

bool onWindows()
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

// True when the linked qtkeychain honours the service name on Windows (0.17.0+), i.e.
// a normal key == service write lands at "service@service" instead of the bare service
bool qtkeychainHonoursService()
{
#if defined(QTKEYCHAIN_LINKED_VERSION)
    return QVersionNumber::fromString(QStringLiteral(QTKEYCHAIN_LINKED_VERSION)) >= QVersionNumber(0, 17, 0);
#else
    return false;
#endif
}

// TargetName a normal CredentialManager write (key == service) resolves to
QString currentTargetName(const QString& service)
{
    return qtkeychainHonoursService() ? service + QLatin1Char('@') + service : service;
}

QString combinedTargetName(const QString& service)
{
    return service + QLatin1Char('@') + service;
}

// Starts the job and pumps events until it finishes; the completion flag is shared so a
// late finish after a timeout cannot write to a dead stack frame. Callers delete the job rather than
// disconnecting it: QtKeychain's queue waits on its own connections to a job that never answered.
bool waitForJob(QKeychain::Job* job)
{
    auto done = std::make_shared<bool>(false);
    QObject::connect(job, &QKeychain::Job::finished, job, [done](QKeychain::Job*) {
        *done = true;
    });
    job->start();
    return QTest::qWaitFor(
            [done]() {
                return *done;
            },
            kWaitMs);
}

// Writes a credential at an explicit TargetName (empty service resolves to the bare key
// on every qtkeychain version)
bool writeTarget(const QString& targetName, const QString& secret)
{
    auto* job = new QKeychain::WritePasswordJob(QString());
    job->setAutoDelete(false);
    job->setKey(targetName);
    job->setTextData(secret);
    const bool ok = waitForJob(job) && job->error() == QKeychain::NoError;
    job->deleteLater();
    return ok;
}

bool readTarget(const QString& targetName, QString* secret = nullptr)
{
    auto* job = new QKeychain::ReadPasswordJob(QString());
    job->setAutoDelete(false);
    job->setKey(targetName);
    const bool ok = waitForJob(job) && job->error() == QKeychain::NoError;
    if (ok && secret) {
        *secret = job->textData();
    }
    job->deleteLater();
    return ok;
}

// Missing entries count as success
bool deleteTarget(const QString& targetName)
{
    auto* job = new QKeychain::DeletePasswordJob(QString());
    job->setAutoDelete(false);
    job->setKey(targetName);
    const bool ok = waitForJob(job) && (job->error() == QKeychain::NoError || job->error() == QKeychain::EntryNotFound);
    job->deleteLater();
    return ok;
}

// The migration chains fire detached cleanup jobs after the user callback, so removals
// need to be polled rather than asserted immediately
bool waitUntilTargetGone(const QString& targetName)
{
    return QTest::qWaitFor(
            [&targetName]() {
                return !readTarget(targetName);
            },
            kWaitMs);
}

bool waitUntilTargetHolds(const QString& targetName, const QString& expectedSecret)
{
    return QTest::qWaitFor(
            [&targetName, &expectedSecret]() {
                QString secret;
                return readTarget(targetName, &secret) && secret == expectedSecret;
            },
            kWaitMs);
}

// Deliberately duplicates the private CredentialManager::generateServiceName: the format
// is persisted in users' credential stores, so this test doubles as a tripwire against
// changing it and orphaning stored entries
// The service/key pairs a test saw, for an assertion message that says what actually happened
QString describeDeletes(const QList<QPair<QString, QString>>& deletes)
{
    QStringList described;
    for (const auto& deletion : deletes) {
        described << QStringLiteral("(%1, %2)").arg(deletion.first, deletion.second);
    }
    return described.join(QStringLiteral(", "));
}

// chosenRoot: a root whose passwords go to the keychain. Resolved here with canonicalFilePath(), not
// MudletApp::folderIdentity(), so a change there is caught too
QString expectedServiceName(const QString& profileName, const QString& key, const QString& chosenRoot = QString())
{
    static const QRegularExpression sanitizePattern(QStringLiteral(R"REGEX([^\w\-\.])REGEX"));

    QString hashed = QStringLiteral("%1:%2").arg(profileName, key);
    if (!chosenRoot.isEmpty()) {
        hashed.prepend(QFileInfo(chosenRoot).canonicalFilePath() + QStringLiteral("/credentials:"));
    }
    const QByteArray data = hashed.toUtf8();
    const QString hashHex = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex()).left(16);

    auto sanitize = [](QString input) {
        input.replace(sanitizePattern, QStringLiteral("_"));
        return input.left(20);
    };

    return QStringLiteral("Mudlet-%1-%2-%3").arg(sanitize(profileName), sanitize(key), hashHex);
}

// Duplicates the private CredentialManager::generateLegacyServiceName for the same reason
QString expectedLegacyServiceName(const QString& profileName, const QString& key)
{
    static const QRegularExpression sanitizePattern(QStringLiteral(R"REGEX([^\w\-\.])REGEX"));

    auto sanitize = [](QString input) {
        input.replace(sanitizePattern, QStringLiteral("_"));
        return input.left(50);
    };

    return QStringLiteral("Mudlet-%1-%2").arg(sanitize(profileName), sanitize(key));
}

struct OperationResult
{
    bool done = false;
    bool success = false;
    QString password;
};

// Callback state is heap-shared so a callback arriving after a timed-out wait is harmless
OperationResult storePassword(CredentialManager& manager, const QString& profile, const QString& key, const QString& password)
{
    auto state = std::make_shared<OperationResult>();
    manager.storePassword(profile, key, password, [state](bool success, const QString&) {
        state->success = success;
        state->done = true;
    });
    QTest::qWaitFor(
            [state]() {
                return state->done;
            },
            kWaitMs);
    return *state;
}

OperationResult retrievePassword(CredentialManager& manager, const QString& profile, const QString& key)
{
    auto state = std::make_shared<OperationResult>();
    manager.retrievePassword(profile, key, [state](bool success, const QString& password, const QString&) {
        state->success = success;
        state->password = password;
        state->done = true;
    });
    QTest::qWaitFor(
            [state]() {
                return state->done;
            },
            kWaitMs);
    return *state;
}

OperationResult removePassword(CredentialManager& manager, const QString& profile, const QString& key)
{
    auto state = std::make_shared<OperationResult>();
    manager.removePassword(profile, key, [state](bool success, const QString&) {
        state->success = success;
        state->done = true;
    });
    QTest::qWaitFor(
            [state]() {
                return state->done;
            },
            kWaitMs);
    return *state;
}

// Writes a credential under an explicit service and key, for the layouts whose service is not empty
bool writeEntry(const QString& service, const QString& key, const QString& secret)
{
    auto* job = new QKeychain::WritePasswordJob(service);
    job->setAutoDelete(false);
    job->setKey(key);
    job->setTextData(secret);
    const bool ok = waitForJob(job) && job->error() == QKeychain::NoError;
    job->deleteLater();
    return ok;
}

bool deleteEntry(const QString& service, const QString& key)
{
    auto* job = new QKeychain::DeletePasswordJob(service);
    job->setAutoDelete(false);
    job->setKey(key);
    const bool ok = waitForJob(job) && (job->error() == QKeychain::NoError || job->error() == QKeychain::EntryNotFound);
    job->deleteLater();
    return ok;
}

// Whether QtKeychain still runs jobs at all. It runs one at a time for the whole process, so a job it
// is still waiting on holds every later one back. A job with no service and no key is answered by
// QtKeychain itself once its turn comes, so this needs no working keychain behind it. Only jobs that
// really reached the store are in that queue - a JobStaller takes its own out before they get there -
// so this says that nothing a test started for real was left holding it.
bool keychainQueueRuns()
{
    auto* job = new QKeychain::ReadPasswordJob(QString());
    job->setAutoDelete(false);
    const bool finished = waitForJob(job);
    job->deleteLater();
    return finished;
}

// Runs one callback ahead of every event already queued, which is how a test reaches the window
// between a job answering and the queued result handler that was posted while it did.
class HighPriorityCall : public QObject
{
public:
    explicit HighPriorityCall(std::function<void()> call)
    : mCall(std::move(call))
    {
    }

    void post() { QCoreApplication::postEvent(this, new QEvent(QEvent::User), Qt::HighEventPriority); }

protected:
    bool event(QEvent* e) override
    {
        if (e->type() == QEvent::User) {
            mCall();
            return true;
        }
        return QObject::event(e);
    }

private:
    std::function<void()> mCall;
};

struct ExpectedRead
{
    QString name;
    QString service;
    QString key;
};

// Every place a lookup of key looks, in order. Spelled out independently of CredentialManager for the
// same reason as expectedServiceName(): these are layouts already in players' keychains.
QList<ExpectedRead> expectedReads(const QString& profileName, const QString& key)
{
    const QString service = expectedServiceName(profileName, key);
    const QString legacyService = expectedLegacyServiceName(profileName, key);
    // The sign-in keys are not read out of a layout older than they are: the colliding format
    // belongs to 4.20.0 and 4.20.1 and the key=account layout to builds before the Windows keychain
    // fix, both of which predate saved sign-ins (#11029).
    const bool olderLayouts = key != QStringLiteral("reconnect") && key != QStringLiteral("reconnect-token");
    QList<ExpectedRead> reads;
    reads.append({QStringLiteral("current format"), service, service});
#if defined(Q_OS_WIN)
    reads.append({QStringLiteral("pre-0.17 naming"), QString(), service});
    if (olderLayouts) {
        reads.append({QStringLiteral("old key=account format"), QString(), key});
    }
#else
    if (olderLayouts) {
        reads.append({QStringLiteral("old key=account format"), service, key});
    }
#endif
    if (key == QStringLiteral("character") || key == QStringLiteral("password")) {
        reads.append({QStringLiteral("pre-4.20.0 format"), QStringLiteral("Mudlet profile"), profileName});
    }
    if (olderLayouts) {
        reads.append({QStringLiteral("colliding format"), legacyService, legacyService});
#if defined(Q_OS_WIN)
        reads.append({QStringLiteral("colliding format under pre-0.17 naming"), QString(), legacyService});
#else
        reads.append({QStringLiteral("colliding format with key=account"), legacyService, key});
#endif
    }
    return reads;
}

// Stands in for the credential store on the jobs it chooses, and records every read.
//
// A job it takes over is never started, so it never reaches the store, and the test alone decides
// when - and whether - it answers: a stalled one simply never does, which is what an unanswered
// unlock prompt or a wedged secret service looks like to CredentialManager. That is also the only
// safe way to do this. A store call cannot be cancelled - qtkeychain hands libsecret, and Apple's
// keychain a dispatch queue, a raw pointer to the running job and keeps nothing to call it off with
// - so a job that has reached the store has to be left to the store to answer and to outlive.
// Faking an answer for one and letting CredentialManager delete it, as this used to, left the store
// writing into freed memory when the real answer landed afterwards. Which of the two answers came
// first was down to the machine: a SIGSEGV on every run where no secret service answers at all, and
// a race anywhere the store does - so it read as flakiness rather than as the crash it was. #10454
// is the same crash reached from the field. Windows was never affected: its backend answers inside
// scheduledStart(), leaving nothing outstanding.
//
// Nothing the staller takes over touches QtKeychain's process-wide queue, so what a real store job
// left running would hold up is checked separately, by keychainQueueRuns().
class JobStaller
{
public:
    // Stalls the nth job of type T started from now on, counting from zero.
    template <typename T>
    void stallNth(int n)
    {
        mShouldStall = [this, n](QKeychain::Job* job) {
            return qobject_cast<T*>(job) && mSeen++ == n;
        };
    }

    // Stalls several jobs of type T by index, counting from zero, so a test can let the store
    // answer one read and refuse the two after it
    template <typename T>
    void stallNths(const QList<int>& indices)
    {
        mShouldStall = [this, indices](QKeychain::Job* job) {
            if (!qobject_cast<T*>(job)) {
                return false;
            }
            return indices.contains(mSeen++);
        };
    }

    template <typename T>
    void stallEvery()
    {
        mShouldStall = [](QKeychain::Job* job) {
            return qobject_cast<T*>(job) != nullptr;
        };
    }

    // Answers every read that is not stalled as finding nothing, so a chain can be walked on a machine
    // whose keychain the test cannot reach.
    void answerOtherReadsNotFound(QKeychain::Error error = QKeychain::EntryNotFound)
    {
        mAnswerReadsNotFound = true;
        mNotFoundError = error;
    }

    // Returns false for every job it takes over, which is what keeps that job away from the store
    std::function<bool(QKeychain::Job*)> hook()
    {
        return [this](QKeychain::Job* job) {
            const bool read = qobject_cast<QKeychain::ReadPasswordJob*>(job);
            if (read) {
                mReads.append({job->service(), job->key()});
            }
            if (qobject_cast<QKeychain::DeletePasswordJob*>(job)) {
                mDeletes.append({job->service(), job->key()});
            }
            if (mShouldStall && mShouldStall(job)) {
                watch(job);
                mStalled.append(job);
                return false;
            }
            if (read && mAnswerReadsNotFound) {
                watch(job);
                QTimer::singleShot(0ms, job, [job, error = mNotFoundError]() {
                    answer(job, error, QStringLiteral("synthetic: nothing found"));
                });
                return false;
            }
            return true;
        };
    }

    const QList<QPair<QString, QString>>& reads() const { return mReads; }
    qsizetype stalledCount() const { return mStalled.size(); }
    const QList<QPair<QString, QString>>& deletes() const { return mDeletes; }

    bool waitForAnyStalled()
    {
        return QTest::qWaitFor(
                [this]() {
                    return !mStalled.isEmpty();
                },
                kWaitMs);
    }

    // The first stalled job, while it is still alive
    QKeychain::Job* waitForStalled() { return waitForAnyStalled() ? mStalled.constFirst().data() : nullptr; }

    // The nth stalled job, counting from zero, once the staller has taken that many over. A test
    // answering one refusal and then waiting again would otherwise be handed the same job back -
    // mStalled keeps what it has taken over, and the one just answered is on its way to deletion -
    // so a chain of refusals has to name which of them it means.
    QKeychain::Job* waitForStalled(int index)
    {
        const bool arrived = QTest::qWaitFor(
                [this, index]() {
                    return mStalled.size() > index;
                },
                kWaitMs);
        if (!arrived) {
            return nullptr;
        }
        return mStalled.at(index).data();
    }

    bool firstStalledAlive() const { return !mStalled.isEmpty() && mStalled.constFirst(); }

    // Answers a job the staller took over, with an error of the test's choosing
    static void answer(QKeychain::Job* job, QKeychain::Error error, const QString& message) { job->emitFinishedWithError(error, message); }

    // Hands a job the staller took over back, to be started for real: the store answers it from
    // then on, and has to be left to outlive it like any other job it holds
    void forget(QKeychain::Job* job)
    {
        mStalled.removeIf([job](const QPointer<QKeychain::Job>& stalled) {
            return stalled == job;
        });
    }

    // Lets every stalled job still alive answer, and reports how many that was
    int release()
    {
        const auto stalled = mStalled;
        int released = 0;
        for (const auto& job : stalled) {
            if (job) {
                answer(job, QKeychain::OtherError, QStringLiteral("synthetic: released by the test"));
                ++released;
            }
        }
        return released;
    }

    // How many answers reached a receiver that is not the manager that started the job. It stands in
    // for QtKeychain's own connection to the job it is running: the executor learns a job finished
    // through that connection alone, so a manager that abandoned a job with a wildcard disconnect()
    // instead of dropping only its own connections would leave every later keychain job in the
    // process queued for good.
    int answersToOtherReceivers() const { return mOutsideAnswers; }

    ~JobStaller() { release(); }

private:
    void watch(QKeychain::Job* job)
    {
        QObject::connect(job, &QKeychain::Job::finished, &mWitness, [this](QKeychain::Job*) {
            ++mOutsideAnswers;
        });
    }

    std::function<bool(QKeychain::Job*)> mShouldStall;
    QList<QPointer<QKeychain::Job>> mStalled;
    QList<QPair<QString, QString>> mReads;
    QList<QPair<QString, QString>> mDeletes;
    QObject mWitness;
    int mOutsideAnswers = 0;
    int mSeen = 0;
    bool mAnswerReadsNotFound = false;
    QKeychain::Error mNotFoundError = QKeychain::EntryNotFound;
};

enum class HiddenCopy { CannotDecrypt, OlderNamedThatStays, FolderHidesIt, CannotBeLookedAt };
enum class Settling { Save, Forget, ForgetPreferredFilesAlone };

struct Answer
{
    int count = 0;
    bool success = false;
    QString password;
    QString error;
};

// Counts every answer, so a lookup answering twice is caught as surely as one never answering
std::shared_ptr<Answer> startRetrieval(CredentialManager& manager, const QString& profile, const QString& key)
{
    auto answer = std::make_shared<Answer>();
    manager.retrievePassword(profile, key, [answer](bool success, QString password, const QString& error) {
        ++answer->count;
        answer->success = success;
        answer->password = password;
        answer->error = error;
    });
    return answer;
}

std::shared_ptr<Answer> startStore(CredentialManager& manager, const QString& profile, const QString& key, const QString& password)
{
    auto answer = std::make_shared<Answer>();
    manager.storePassword(profile, key, password, [answer](bool success, const QString& error) {
        ++answer->count;
        answer->success = success;
        answer->error = error;
    });
    return answer;
}

// Forgets as removePassword() does, answering every keychain delete it makes with deleteAnswer
std::shared_ptr<Answer> forgetAnsweringDeletes(CredentialManager& manager,
                                               JobStaller& staller,
                                               const QString& profile,
                                               const QString& key,
                                               const CredentialManager::StoreScope scope = CredentialManager::StoreScope::EveryStore,
                                               const QKeychain::Error deleteAnswer = QKeychain::EntryNotFound)
{
    staller.stallEvery<QKeychain::DeletePasswordJob>();
    auto answer = std::make_shared<Answer>();
    manager.removePassword(
            profile,
            key,
            [answer](bool success, const QString& error) {
                ++answer->count;
                answer->success = success;
                answer->error = error;
            },
            scope);
    int answered = 0;
    // The caller's waitForAnswer() says whether it came
    static_cast<void>(QTest::qWaitFor(
            [&staller, &answered, answer, deleteAnswer]() {
                while (staller.stalledCount() > answered) {
                    JobStaller::answer(staller.waitForStalled(answered++), deleteAnswer, QStringLiteral("synthetic: the keychain's answer to a delete"));
                }
                return answer->count > 0;
            },
            kWaitMs));
    return answer;
}

bool writeEncryptedCopy(const QString& path, const QString& profile, const QString& secret)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(SecureStringUtils::encryptStringForProfile(secret, profile).toUtf8()) > 0;
}

bool waitForAnswer(const std::shared_ptr<Answer>& answer, int timeoutMs = kWaitMs)
{
    return QTest::qWaitFor(
            [answer]() {
                return answer->count > 0;
            },
            timeoutMs);
}

struct TimedAnswers
{
    std::shared_ptr<Answer> first = std::make_shared<Answer>();
    bool timedOut = false;
    CredentialManager::ReadOutcome outcome = CredentialManager::ReadOutcome::Found;
    std::shared_ptr<Answer> late = std::make_shared<Answer>();
};

// Both answers of a lookup that can still be answered after its deadline, each counted
std::shared_ptr<TimedAnswers> startTimedRetrieval(CredentialManager& manager, const QString& profile, const QString& key, QObject* lateContext)
{
    auto answers = std::make_shared<TimedAnswers>();
    manager.retrievePassword(
            profile,
            key,
            [answers](bool success, QString password, const QString& error, CredentialManager::ReadOutcome outcome) {
                ++answers->first->count;
                answers->first->success = success;
                answers->first->password = password;
                answers->first->error = error;
                answers->outcome = outcome;
                answers->timedOut = outcome == CredentialManager::ReadOutcome::TimedOut;
            },
            lateContext,
            [answers](bool success, QString password, const QString& error) {
                ++answers->late->count;
                answers->late->success = success;
                answers->late->password = password;
                answers->late->error = error;
            });
    return answers;
}

} // namespace

Q_DECLARE_METATYPE(QKeychain::Error)
Q_DECLARE_METATYPE(CredentialManager::ReadOutcome)
Q_DECLARE_METATYPE(HiddenCopy)
Q_DECLARE_METATYPE(Settling)

void CredentialManagerKeychainTest::initTestCase()
{
    // CredentialManagerTest forces file storage via this variable; this test exists to
    // exercise the real credential store, so make sure it is not inherited from the
    // environment
    qunsetenv("MUDLET_TEST_MODE");

    // The real store is the subject here, but CredentialManager falls back to files
    // under QStandardPaths::AppConfigLocation whenever the keychain is unavailable -
    // which for a QTEST_MAIN program is $HOME/.config/CredentialManagerKeychainTest.
    // Same recipe as CredentialManagerTest, and like it this only takes effect where
    // QStandardPaths honours XDG.
    QVERIFY(mConfigDir.isValid());
    qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

#if defined(Q_OS_MACOS)
    // No probe here, and so no test that writes to the store. The suite hands every test a
    // CFFIXED_USER_HOME of its own (test/CMakeLists.txt) and the Security framework resolves the
    // login keychain from that home, so there is no keychain to write to - and a write does not
    // fail, it asks whoever ran the suite whether to reset their keychain to defaults. Nothing in
    // a test run should put that question in front of anyone. The reads the lookup-chain cases
    // make are answered as "not found" without any of that, so those cases still run; the ones
    // that need a store of their own skip, and Linux and Windows CI cover them.
    mStoreAvailable = false;
#else
    // On every platform: the Windows migration tests need it, and so do the tests of the lookup chain
    // that need its reads to be answered.
    const QString probe = QStringLiteral("MudletKCTest-probe-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8));
    mStoreAvailable = writeTarget(probe, QStringLiteral("probe")) && deleteTarget(probe);

    if (!mStoreAvailable) {
        qWarning() << "CredentialManagerKeychainTest: credential store unavailable, tests will skip";
    }
#endif
}

void CredentialManagerKeychainTest::init()
{
    // Process-wide, so a case that had a read refused would otherwise trim the chain of the
    // cases after it
    CredentialManager::forgetStoreRefusal();
    CredentialManager::profileStorageOverrideForTesting().reset();
    CredentialManager::readTextForTesting().reset();
    MudletApp::setStorePasswordsSecurely(true);
    const QString runId = QUuid::createUuid().toString(QUuid::Id128).left(8);
    mProfile = QStringLiteral("MudletKCTest-%1").arg(runId);
    mKey = QStringLiteral("kctest_%1").arg(runId);
}

void CredentialManagerKeychainTest::cleanup()
{
    // A test that chose a root would otherwise name every later test's entries after it
    MudletApp::setConfigPath(QString());
    // On every platform, and here rather than only at the end of each test, so a test that fails part
    // way through leaves nothing behind in the keychain of whoever runs the suite. The encrypted-file
    // copy goes too: it lives under AppConfigLocation, which does not follow XDG_CONFIG_HOME everywhere.
    CredentialManager::removeCredential(mProfile, mKey);
    if (!mStoreAvailable) {
        return;
    }

    // Every entry a test could have created or migrated to
    const QString service = expectedServiceName(mProfile, mKey);
    const QString legacyService = expectedLegacyServiceName(mProfile, mKey);
    if (onWindows()) {
        const QStringList targets = {service, combinedTargetName(service), legacyService, combinedTargetName(legacyService), mKey};
        for (const QString& target : targets) {
            deleteTarget(target);
        }
        return;
    }
    const QList<QPair<QString, QString>> entries = {{service, service}, {service, mKey}, {legacyService, legacyService}, {legacyService, mKey}};
    for (const auto& [entryService, entryKey] : entries) {
        deleteEntry(entryService, entryKey);
    }
}

void CredentialManagerKeychainTest::testRoundTrip()
{
    if (!onWindows()) {
        QSKIP("Windows-only: exercises the Windows Credential Manager naming schemes");
    }
    if (!mStoreAvailable) {
        QSKIP("Windows credential store unavailable in this environment");
    }

    const QString secret = QStringLiteral("roundtrip-secret");
    CredentialManager manager;

    QVERIFY(storePassword(manager, mProfile, mKey, secret).success);

    // The entry must be in the credential store itself, not the file fallback
    const QString service = expectedServiceName(mProfile, mKey);
    QString stored;
    QVERIFY(readTarget(currentTargetName(service), &stored));
    QCOMPARE(stored, secret);

    const OperationResult retrieved = retrievePassword(manager, mProfile, mKey);
    QVERIFY(retrieved.success);
    QCOMPARE(retrieved.password, secret);

    QVERIFY(removePassword(manager, mProfile, mKey).success);
    QVERIFY(waitUntilTargetGone(currentTargetName(service)));
    QVERIFY(waitUntilTargetGone(service));
}

void CredentialManagerKeychainTest::testBareEntryMigration()
{
    if (!onWindows()) {
        QSKIP("Windows-only: exercises the Windows Credential Manager naming schemes");
    }
    if (!mStoreAvailable) {
        QSKIP("Windows credential store unavailable in this environment");
    }

    // Plant an entry in the pre-0.17 layout: TargetName == service
    const QString secret = QStringLiteral("bare-entry-secret");
    const QString service = expectedServiceName(mProfile, mKey);
    QVERIFY(writeTarget(service, secret));

    CredentialManager manager;
    const OperationResult retrieved = retrievePassword(manager, mProfile, mKey);
    QVERIFY(retrieved.success);
    QCOMPARE(retrieved.password, secret);

    if (qtkeychainHonoursService()) {
        // 0.17+: the compat migration re-stores under "service@service" and the
        // version-gated cleanup removes the bare entry
        QVERIFY(waitUntilTargetHolds(combinedTargetName(service), secret));
        QVERIFY(waitUntilTargetGone(service));
    } else {
        // pre-0.17: the primary read already resolves to the bare name, so the
        // migration stays dormant and the entry is untouched
        QString still;
        QVERIFY(readTarget(service, &still));
        QCOMPARE(still, secret);
    }

    // A second retrieve must serve the value from the migrated location and must not re-fire
    // the migration (which would churn the store or resurrect the old entry)
    const OperationResult again = retrievePassword(manager, mProfile, mKey);
    QVERIFY(again.success);
    QCOMPARE(again.password, secret);
    if (qtkeychainHonoursService()) {
        QVERIFY(!readTarget(service));
        QVERIFY(waitUntilTargetHolds(combinedTargetName(service), secret));
    }
}

void CredentialManagerKeychainTest::testRemoveSweepsBareEntry()
{
    if (!onWindows()) {
        QSKIP("Windows-only: exercises the Windows Credential Manager naming schemes");
    }
    if (!mStoreAvailable) {
        QSKIP("Windows credential store unavailable in this environment");
    }

    // A deleted password must not be resurrected by the compat migration: removing before
    // any read has migrated the bare entry must sweep the bare TargetName as well
    const QString secret = QStringLiteral("swept-secret");
    const QString service = expectedServiceName(mProfile, mKey);
    QVERIFY(writeTarget(service, secret));

    CredentialManager manager;
    QVERIFY(removePassword(manager, mProfile, mKey).success);
    QVERIFY(waitUntilTargetGone(service));

    const OperationResult retrieved = retrievePassword(manager, mProfile, mKey);
    QVERIFY(!retrieved.success);
    QVERIFY(retrieved.password.isEmpty());
}

void CredentialManagerKeychainTest::testOldFormatMigration()
{
    if (!onWindows()) {
        QSKIP("Windows-only: exercises the Windows Credential Manager naming schemes");
    }
    if (!mStoreAvailable) {
        QSKIP("Windows credential store unavailable in this environment");
    }

    // Plant an entry in the old (pre-Windows-fix) layout: TargetName == account, the
    // format that made all profiles share one credential
    const QString secret = QStringLiteral("old-format-secret");
    QVERIFY(writeTarget(mKey, secret));

    CredentialManager manager;
    const OperationResult retrieved = retrievePassword(manager, mProfile, mKey);
    QVERIFY(retrieved.success);
    QCOMPARE(retrieved.password, secret);

    // Migrated to the current naming scheme and the old entry cleaned up
    const QString service = expectedServiceName(mProfile, mKey);
    QVERIFY(waitUntilTargetHolds(currentTargetName(service), secret));
    QVERIFY(waitUntilTargetGone(mKey));

    // A second retrieve serves from the migrated entry and does not re-create the old one
    const OperationResult again = retrievePassword(manager, mProfile, mKey);
    QVERIFY(again.success);
    QCOMPARE(again.password, secret);
    QVERIFY(!readTarget(mKey));
}

void CredentialManagerKeychainTest::testCollidingFormatRecovery()
{
    if (!onWindows()) {
        QSKIP("Windows-only: exercises the Windows Credential Manager naming schemes");
    }
    if (!mStoreAvailable) {
        QSKIP("Windows credential store unavailable in this environment");
    }

    // Plant an entry in the colliding legacy layout: TargetName == legacy service name
    // (as written by a pre-0.17 build of Mudlet 4.20.x)
    const QString secret = QStringLiteral("colliding-secret");
    const QString legacyService = expectedLegacyServiceName(mProfile, mKey);
    QVERIFY(writeTarget(legacyService, secret));

    CredentialManager manager;
    const OperationResult retrieved = retrievePassword(manager, mProfile, mKey);
    QVERIFY(retrieved.success);
    QCOMPARE(retrieved.password, secret);

    // Re-stored under the hash-based name; the colliding entry is removed afterwards
    // (version-gated on APP_VERSION > 4.20.1, which holds for this build)
    const QString service = expectedServiceName(mProfile, mKey);
    QVERIFY(waitUntilTargetHolds(currentTargetName(service), secret));
    QVERIFY(waitUntilTargetGone(legacyService));

    if (qtkeychainHonoursService()) {
        QVERIFY(waitUntilTargetGone(combinedTargetName(legacyService)));
    }

    // A second retrieve serves from the migrated entry and does not re-create the colliding one
    const OperationResult again = retrievePassword(manager, mProfile, mKey);
    QVERIFY(again.success);
    QCOMPARE(again.password, secret);
    QVERIFY(!readTarget(legacyService));
}

void CredentialManagerKeychainTest::testALookupAnswersWhenItsFirstReadStalls()
{
    // Needs no working keychain: nothing but the stalled read and QtKeychain's queue is involved.
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY2(waitForAnswer(answer), "a lookup whose keychain read never answers must still answer its caller");
    QVERIFY2(staller.waitForAnyStalled(), "the read meant to stall was never started, so this run tested nothing");
    QVERIFY(!answer->success);
    QCOMPARE(answer->error, QStringLiteral("Operation timed out"));

    // The read is still waiting on the keychain, and a backend that answers a deleted job reads freed
    // memory, so it has to be left to finish on its own.
    QTest::qWait(100ms);
    QVERIFY2(staller.firstStalledAlive(), "a read still waiting on the keychain was deleted");

    // Once the keychain answers, the read deletes itself - and the answer has to reach receivers other
    // than the manager, since QtKeychain's queue moves on through its own connection to the job.
    const int released = staller.release();
    QCOMPARE(staller.answersToOtherReceivers(), released);
    QVERIFY2(keychainQueueRuns(), "QtKeychain's queue did not move on once the stalled read answered");
    QTRY_VERIFY2(!staller.firstStalledAlive(), "a read that answered after its lookup gave up was never deleted");
    QCOMPARE(answer->count, 1);
}

// Reading the file needs nothing from the keychain, and the next lookup would stall at the same read,
// so a password kept only there was never found while the keychain did not answer
void CredentialManagerKeychainTest::testALookupWhoseKeychainStallsStillReadsTheEncryptedFile()
{
    const QString secret = QStringLiteral("file-secret-behind-a-stall");
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, secret));
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;

    const auto answer = startRetrieval(manager, mProfile, mKey);
    const bool answered = waitForAnswer(answer);
    CredentialManager::removeCredential(mProfile, mKey);

    QVERIFY2(answered, "a lookup whose keychain read never answers must still answer its caller");
    QVERIFY2(staller.waitForAnyStalled(), "the read meant to stall was never started, so this run tested nothing");
    QVERIFY2(answer->success, qPrintable(answer->error));
    QCOMPARE(answer->password, secret);

    staller.release();
    QTRY_VERIFY2(!staller.firstStalledAlive(), "a read that answered after its lookup was answered was never deleted");
    QCOMPARE(answer->count, 1);
}

void CredentialManagerKeychainTest::testALookupAnswersWhicheverLaterReadStalls_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<int>("stalledRead");

    // The reads depend only on the key, so any profile gives the same rows
    for (const QString& key : {QStringLiteral("character"), QStringLiteral("kctest")}) {
        const QList<ExpectedRead> reads = expectedReads(QStringLiteral("profile"), key);
        for (int read = 1; read < reads.size(); ++read) {
            QTest::addRow("%s: %s", qPrintable(key), qPrintable(reads.at(read).name)) << key << read;
        }
    }
}

void CredentialManagerKeychainTest::testALookupAnswersWhicheverLaterReadStalls()
{
    QFETCH(QString, key);
    QFETCH(int, stalledRead);
    if (key != QStringLiteral("character")) {
        key = mKey;
    }

    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(stalledRead);
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    // Long enough for the reads before the stalled one, which the staller answers a posted event at a
    // time. Too short shows up as a read count below the one that was meant to stall, not as a pass.
    manager.mOperationTimeoutMs = 1000;

    const auto answer = startRetrieval(manager, mProfile, key);
    QVERIFY2(waitForAnswer(answer), "a lookup whose keychain read never answers must still answer its caller");
    QVERIFY2(staller.waitForAnyStalled(), "the read meant to stall was never started, so this run tested nothing");
    QVERIFY(!answer->success);
    QCOMPARE(answer->error, QStringLiteral("Operation timed out"));
    QCOMPARE(staller.reads().size(), stalledRead + 1);
    QVERIFY2(staller.firstStalledAlive(), "a read still waiting on the keychain was deleted");

    const int released = staller.release();
    QCOMPARE(staller.answersToOtherReceivers(), released + stalledRead);
    QVERIFY2(keychainQueueRuns(), "QtKeychain's queue did not move on once the stalled read answered");
    QCOMPARE(answer->count, 1);
}

void CredentialManagerKeychainTest::testALookupReadsEachPlaceOnceInOrder_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QKeychain::Error>("nothingFound");
    QTest::newRow("a key with a pre-4.20.0 format") << QStringLiteral("character") << QKeychain::EntryNotFound;
    QTest::newRow("a key without one") << QString() << QKeychain::EntryNotFound;
    // No keychain service at all is somewhere nothing is stored, not a keychain refusing to answer
    QTest::newRow("no keychain service") << QString() << QKeychain::NoBackendAvailable;
}

void CredentialManagerKeychainTest::testALookupReadsEachPlaceOnceInOrder()
{
    QFETCH(QString, key);
    QFETCH(QKeychain::Error, nothingFound);
    if (key.isEmpty()) {
        key = mKey;
    }

    JobStaller recorder;
    recorder.answerOtherReadsNotFound(nothingFound);
    CredentialManager manager;
    manager.mJobStartHook = recorder.hook();

    // A profile with nothing saved anywhere, so the lookup goes all the way down the chain
    const auto answer = startRetrieval(manager, mProfile, key);
    QVERIFY(waitForAnswer(answer));
    QVERIFY(!answer->success);
    QCOMPARE(answer->error, QStringLiteral("No stored credentials found for profile %1").arg(mProfile));

    QList<QPair<QString, QString>> expected;
    for (const auto& read : expectedReads(mProfile, key)) {
        expected.append({read.service, read.key});
    }
    QCOMPARE(recorder.reads(), expected);
    QTest::qWait(200ms);
    QCOMPARE(answer->count, 1);
}

void CredentialManagerKeychainTest::testATimedOutRemovalDoesNotStopLaterKeychainJobs()
{
    // Every removal: once the first times out, removePassword() goes on to remove the colliding-format
    // entry, and that job must not be left holding the queue either
    JobStaller staller;
    staller.stallEvery<QKeychain::DeletePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;

    auto answered = std::make_shared<bool>(false);
    auto succeeded = std::make_shared<bool>(true);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("abandoned the keychain removal for profile \"%1\"").arg(QRegularExpression::escape(mProfile))));
    manager.removePassword(mProfile, mKey, [answered, succeeded](bool success, const QString&) {
        *answered = true;
        *succeeded = success;
    });
    QVERIFY2(QTest::qWaitFor(
                     [answered]() {
                         return *answered;
                     },
                     kWaitMs),
             "a removal whose keychain job never answers must still answer its caller");
    QVERIFY(!*succeeded);
    QTest::qWait(3 * manager.mOperationTimeoutMs);
    QVERIFY2(staller.firstStalledAlive(), "a removal still waiting on the keychain was deleted");

    // Abandoning the job must leave every connection to it that is not the manager's own: QtKeychain's
    // queue moves on only through its connection to the job it is running, so a wildcard disconnect()
    // would leave every later keychain job in the process queued for good. The staller's own receiver
    // stands in for that connection, since the job it holds never reached the store to be queued there.
    const int released = staller.release();
    QCOMPARE(released, 2);
    QVERIFY2(staller.answersToOtherReceivers() == released, "a timed-out removal dropped connections to its job that were not its own, so QtKeychain's queue would never learn the job finished");
    QVERIFY2(keychainQueueRuns(), "a timed-out removal stopped every later keychain job from running");
}

// A timed-out operation's callback may start another one on the same manager - dlgConnectionProfiles
// chains keychain removals that way. The cleanup of the timed-out operation has to finish before the
// callback runs, or it tears down the operation the callback just started and that one never answers.
// The missing answer is what this pins; #9072's use-after-free is the same clearing destroying the
// std::function still running.
void CredentialManagerKeychainTest::testAStoreStartedFromATimedOutCallbackIsNotTornDownWithIt()
{
    JobStaller staller;
    staller.stallEvery<QKeychain::WritePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;

    const auto first = std::make_shared<Answer>();
    const auto second = std::make_shared<Answer>();
    const QString profile = mProfile;
    const QString secondKey = mKey + QStringLiteral("-second");

    manager.storePassword(mProfile, mKey, QStringLiteral("secret"), [&manager, first, second, profile, secondKey](bool success, const QString& error) {
        ++first->count;
        first->success = success;
        first->error = error;
        // Must be the last statement: storeCredential() begins by cleaning up the current
        // operation, which in a regression clears the callback running here, so anything after it
        // reads freed memory
        manager.storePassword(profile, secondKey, QStringLiteral("secret-again"), [second](bool laterSuccess, const QString& laterError) {
            ++second->count;
            second->success = laterSuccess;
            second->error = laterError;
        });
    });

    QVERIFY2(waitForAnswer(first), "a store whose keychain job never answers must still answer its caller");
    QVERIFY(!first->success);
    QCOMPARE(first->error, QStringLiteral("Operation timed out"));
    QVERIFY2(waitForAnswer(second), "the store started from the timed-out callback was torn down by the cleanup of the operation that timed out, so it never answered - see #9072");
    QVERIFY(!second->success);
    QCOMPARE(second->error, QStringLiteral("Operation timed out"));
    QTest::qWait(200ms);
    QCOMPARE(first->count, 1);
    QCOMPARE(second->count, 1);
}

void CredentialManagerKeychainTest::testALookupIsNotDisturbedByAnotherOnTheSameManager()
{
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 2000;

    const auto first = startRetrieval(manager, mProfile, mKey);
    QVERIFY(staller.waitForAnyStalled());
    // A second lookup on the same manager while the first is still outstanding
    const auto second = startRetrieval(manager, mProfile + QStringLiteral("-second"), mKey);

    // Each has to reach its own answer: the stalled one its deadline, the other the end of its chain
    QVERIFY2(waitForAnswer(second), "a lookup never answered while another was outstanding on its manager");
    QCOMPARE(second->error, QStringLiteral("No stored credentials found for profile %1").arg(mProfile + QStringLiteral("-second")));
    QVERIFY2(waitForAnswer(first), "the stalled lookup never answered once another lookup started on its manager");
    QCOMPARE(first->error, QStringLiteral("Operation timed out"));
    staller.release();
    QTest::qWait(200ms);
    QCOMPARE(first->count, 1);
    QCOMPARE(second->count, 1);
}

void CredentialManagerKeychainTest::testDeletingAManagerMidLookupLeavesItsReadToFinish()
{
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    auto manager = std::make_unique<CredentialManager>();
    manager->mJobStartHook = staller.hook();

    const auto answer = startRetrieval(*manager, mProfile, mKey);
    QVERIFY(staller.waitForAnyStalled());
    // As a dialog does when it closes on a lookup still in progress. Checked before any event runs: a
    // manager's children are deleted synchronously as it is destroyed, whereas a detached read deletes
    // itself only once the keychain answers - which a real backend may well do moments later.
    manager.reset();
    QVERIFY2(staller.firstStalledAlive(), "a read still waiting on the keychain was deleted along with its manager");
    QTest::qWait(100ms);
    QCOMPARE(answer->count, 0);

    staller.release();
    QVERIFY2(keychainQueueRuns(), "QtKeychain's queue did not move on once the orphaned read answered");
    QTRY_VERIFY2(!staller.firstStalledAlive(), "an orphaned read was never deleted once it answered");
    QCOMPARE(answer->count, 0);
}

void CredentialManagerKeychainTest::testAWriteTheKeychainHasAnsweredIsNotOrphanedWithItsPassword()
{
    // The window between a job answering and its result handler running: the handler is queued, so
    // closing the dialog that started the write drops it along with the manager. QtKeychain will not
    // delete the job either - Job::emitFinished() read autoDelete() as the job answered - so nothing
    // is left to, and it still carries the password it was given.
#if defined(Q_OS_MACOS)
    QSKIP("the write has to reach the store, and on macOS that offers to reset the keychain; see initTestCase()");
#endif
    auto manager = std::make_unique<CredentialManager>();
    QPointer<QKeychain::Job> writeJob;
    bool handlerRan = false;
    bool managerGone = false;
    HighPriorityCall closeTheDialog([&manager, &managerGone]() {
        manager.reset();
        managerGone = true;
    });
    manager->mJobStartHook = [&writeJob, &closeTheDialog](QKeychain::Job* job) {
        writeJob = job;
        // Queued behind the result handler's own posted call but ahead of it, so the manager goes away
        // once the keychain has really answered and QtKeychain is done with the job
        QObject::connect(job, &QKeychain::Job::finished, &closeTheDialog, &HighPriorityCall::post, Qt::DirectConnection);
        return true;
    };

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("abandoned the keychain write for profile \"%1\"").arg(QRegularExpression::escape(mProfile))));
    manager->storePassword(mProfile, mKey, QStringLiteral("orphan-secret"), [&handlerRan](bool, const QString&) {
        handlerRan = true;
    });

    QTRY_VERIFY2(managerGone, "the keychain never answered the write, so this run tested nothing");
    QVERIFY2(!handlerRan, "the write was handled before its manager went away, so the window this test needs never happened");
    QTRY_VERIFY2(!writeJob, "a write the keychain had answered was left behind when its manager went away, with the password still in it");
    QVERIFY2(keychainQueueRuns(), "deleting the answered write stopped every later keychain job from running");
}

void CredentialManagerKeychainTest::testACallbackThatFlushesDeferredDeletesDoesNotDeleteTheAnsweringRead()
{
    // Loading a profile flushes deferred deletes (Host.cpp), and the profiles dialog loads one straight
    // from the lookup's callback - which runs while the read that answered is still emitting finished().
    // A read already marked for deletion by then is freed underneath QtKeychain, which touches it again
    // once the signal returns.
    JobStaller recorder;
    recorder.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = recorder.hook();

    auto answered = std::make_shared<bool>(false);
    manager.retrievePassword(mProfile, mKey, [answered](bool, QString, const QString&) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        *answered = true;
    });
    QVERIFY2(QTest::qWaitFor(
                     [answered]() {
                         return *answered;
                     },
                     kWaitMs),
             "the lookup never answered");
    QTest::qWait(100ms);
}

void CredentialManagerKeychainTest::testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat_data()
{
    QTest::addColumn<bool>("firstReadRefused");
    QTest::newRow("nothing in the keychain") << false;
    // A refused read is reported only when nothing is found, not in place of a password that is
    QTest::newRow("a keychain read refused") << true;
}

void CredentialManagerKeychainTest::testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat()
{
    QFETCH(bool, firstReadRefused);
    const QString secret = QStringLiteral("file-secret");
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, secret));

    JobStaller recorder;
    if (firstReadRefused) {
        recorder.stallNth<QKeychain::ReadPasswordJob>(0);
    }
    recorder.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = recorder.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    if (firstReadRefused) {
        QKeychain::Job* firstRead = recorder.waitForStalled();
        QVERIFY(firstRead);
        JobStaller::answer(firstRead, QKeychain::AccessDenied, QStringLiteral("synthetic: access refused"));
    }
    const bool answered = waitForAnswer(answer);
    CredentialManager::removeCredential(mProfile, mKey);

    QVERIFY(answered);
    QVERIFY(answer->success);
    QCOMPARE(answer->password, secret);
    // The colliding format may hold a password another profile shares, so a lookup that finds this
    // profile's own in the file must stop there.
    QList<QPair<QString, QString>> expected;
    for (const auto& read : expectedReads(mProfile, mKey)) {
        if (!read.name.startsWith(QStringLiteral("colliding format"))) {
            expected.append({read.service, read.key});
        }
    }
    QCOMPARE(recorder.reads(), expected);
}

void CredentialManagerKeychainTest::testARecoveredPasswordIsReturnedWhileItsMigrationStalls()
{
    // An entry in the old key=account layout, which a lookup recovers and then re-files
#if defined(Q_OS_WIN)
    const QString plantedService;
#else
    const QString plantedService = expectedServiceName(mProfile, mKey);
#endif
    if (!mStoreAvailable) {
        QSKIP("credential store unavailable in this environment");
    }
    const QString secret = QStringLiteral("old-format-secret");
    QVERIFY(writeEntry(plantedService, mKey, secret));

    auto staller = std::make_unique<JobStaller>();
    staller->stallEvery<QKeychain::WritePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller->hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    const bool answered = waitForAnswer(answer, 5000);
    QKeychain::Job* migration = staller->waitForStalled();
    const QString migratedService = migration ? migration->service() : QString();
    const QString migratedKey = migration ? migration->key() : QString();

    // Let the stalled migration answer, so it stops holding QtKeychain's queue
    manager.mJobStartHook = nullptr;
    staller.reset();
    const QString service = expectedServiceName(mProfile, mKey);
    deleteEntry(plantedService, mKey);
    deleteEntry(service, service);

    QVERIFY2(answered, "a recovered password must not wait for the write that re-files it");
    QVERIFY(answer->success);
    QCOMPARE(answer->password, secret);
    QVERIFY2(migratedService == service && migratedKey == service, "a recovered password must be re-filed under the current name");
}

void CredentialManagerKeychainTest::testARecoveredPasswordSurvivesAKeychainThatRefusesToStoreIt()
{
    // An entry in the colliding layout. Recovering it stores the password under the current name and
    // then removes the colliding entry.
    const QString legacyService = expectedLegacyServiceName(mProfile, mKey);
#if defined(Q_OS_WIN)
    const QString plantedService;
#else
    const QString plantedService = legacyService;
#endif
    if (!mStoreAvailable) {
        QSKIP("credential store unavailable in this environment");
    }
    const QString secret = QStringLiteral("colliding-secret");
    QVERIFY(writeEntry(plantedService, legacyService, secret));

    auto staller = std::make_unique<JobStaller>();
    staller->stallEvery<QKeychain::WritePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller->hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QVERIFY(answer->success);
    QCOMPARE(answer->password, secret);

    // The keychain refuses the store, so the password goes to the encrypted file instead - and the
    // colliding entry is still removed afterwards, since the password is safely somewhere else.
    QKeychain::Job* store = staller->waitForStalled();
    QVERIFY2(store, "recovering the colliding entry never tried to store it under the current name");
    QCOMPARE(store->service(), expectedServiceName(mProfile, mKey));
    JobStaller::answer(store, QKeychain::OtherError, QStringLiteral("synthetic: keychain refused the write"));
    const bool swept = QTest::qWaitFor(
            [&plantedService, &legacyService]() {
                auto* job = new QKeychain::ReadPasswordJob(plantedService);
                job->setAutoDelete(false);
                job->setKey(legacyService);
                const bool gone = waitForJob(job) && job->error() == QKeychain::EntryNotFound;
                job->deleteLater();
                return gone;
            },
            kWaitMs);
    const QString filed = CredentialManager::retrieveCredential(mProfile, mKey);

    manager.mJobStartHook = nullptr;
    staller.reset();
    const QString service = expectedServiceName(mProfile, mKey);
    deleteEntry(plantedService, legacyService);
    deleteEntry(service, service);
    CredentialManager::removeCredential(mProfile, mKey);

    QVERIFY2(swept, "the colliding entry was never removed after its password was re-filed");
    QVERIFY2(filed == secret, "removing the colliding entry also removed the file its password had just been moved to, so the password ended up nowhere");
}

// The file fallback still saves the password, but the caller has to be able to tell the player
// it is not in the keychain they chose (#11030)
void CredentialManagerKeychainTest::testAStoreTheKeychainRefusesSaysItWentToTheFile()
{
    JobStaller staller;
    staller.stallEvery<QKeychain::WritePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startStore(manager, mProfile, mKey, QStringLiteral("refused-secret"));
    QKeychain::Job* store = staller.waitForStalled();
    QVERIFY2(store, "the store never reached the keychain, so this run tested nothing");
    JobStaller::answer(store, QKeychain::AccessDenied, QStringLiteral("synthetic: keychain refused the write"));

    QVERIFY(waitForAnswer(answer));
    QVERIFY2(answer->success, qPrintable(answer->error));
    QCOMPARE(CredentialManager::retrieveCredential(mProfile, mKey), QStringLiteral("refused-secret"));
    QVERIFY2(manager.keychainRefusal().contains(QStringLiteral("synthetic: keychain refused the write")),
             "a password the keychain refused was saved in the profile's encrypted file without the caller being told");

    // The next store on the same manager reports only its own outcome
    CredentialManager::profileStorageOverrideForTesting() = true;
    const auto second = startStore(manager, mProfile, mKey, QStringLiteral("file-secret"));
    QVERIFY(waitForAnswer(second));
    QVERIFY2(second->success, qPrintable(second->error));
    QVERIFY2(manager.keychainRefusal().isEmpty(), "a store that never asked the keychain still reported the previous store's refusal");
}

void CredentialManagerKeychainTest::testAKeychainErrorIsReportedRatherThanNothingFound_data()
{
    QTest::addColumn<int>("refusedRead");
    QTest::newRow("the first read refused") << 0;
    QTest::newRow("the last read refused") << static_cast<int>(expectedReads(QStringLiteral("profile"), QStringLiteral("kctest")).size() - 1);
}

void CredentialManagerKeychainTest::testAKeychainErrorIsReportedRatherThanNothingFound()
{
    QFETCH(int, refusedRead);
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(refusedRead);
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QKeychain::Job* refused = staller.waitForStalled();
    QVERIFY(refused);
    JobStaller::answer(refused, QKeychain::AccessDenied, QStringLiteral("synthetic: access refused"));

    QVERIFY(waitForAnswer(answer));
    QVERIFY(!answer->success);
    QVERIFY2(answer->error.contains(QStringLiteral("synthetic: access refused")),
             qPrintable(QStringLiteral("a keychain that refused the read may still hold the password, but the lookup said: %1").arg(answer->error)));
    // A refused read is one place the password is not known to be missing from, not a reason to stop
    // looking in the others: the read after it is answered here, so the chain runs to the end. Only
    // refusals with nothing answered between them are the store itself saying no (#11029).
    QCOMPARE(staller.reads().size(), expectedReads(mProfile, mKey).size());
}

namespace {
bool storeThenCorrupt(const QString& profile, const QString& key)
{
    if (!CredentialManager::storeCredential(profile, key, QStringLiteral("soon_unreadable"))) {
        return false;
    }
    QFile file(QStringLiteral("%1/profiles/%2/passwords/%3").arg(MudletApp::credentialFilesRoot(), profile, key));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const bool written = file.write("this is not anything Mudlet encrypted") > 0;
    file.close();
    return written;
}
} // namespace

void CredentialManagerKeychainTest::testAnUndecryptableFileIsReportedRatherThanNothingFound_data()
{
    QTest::addColumn<QKeychain::Error>("keychainAnswer");
    QTest::newRow("nothing in the keychain") << QKeychain::EntryNotFound;
    // A keychain that fails may still hold the password, but the field is empty either way, and
    // the damaged file is the one thing the player can fix by typing it again
    QTest::newRow("a keychain that fails") << QKeychain::OtherError;
    QTest::newRow("a keychain that refuses") << QKeychain::AccessDenied;
}

// A file that is there but cannot be decrypted is a password the player has to type again, not
// one that was never saved
void CredentialManagerKeychainTest::testAnUndecryptableFileIsReportedRatherThanNothingFound()
{
    QFETCH(QKeychain::Error, keychainAnswer);
    QVERIFY(storeThenCorrupt(mProfile, mKey));

    JobStaller staller;
    staller.answerOtherReadsNotFound(keychainAnswer);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QVERIFY(!answer->success);
    QCOMPARE(answer->error, CredentialManager::unreadableFileError(mProfile));
}

// The lookup straight after a refusal reads only the file, which is no reason to blame the keychain
// for a file that cannot be decrypted
void CredentialManagerKeychainTest::testAnUndecryptableFileIsReportedWhileTheKeychainStillRefuses()
{
    QVERIFY(storeThenCorrupt(mProfile, mKey));
    {
        JobStaller staller;
        staller.answerOtherReadsNotFound(QKeychain::AccessDenied);
        CredentialManager manager;
        manager.mJobStartHook = staller.hook();
        QVERIFY(waitForAnswer(startRetrieval(manager, mProfile, mKey)));
    }

    JobStaller staller;
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QVERIFY2(staller.reads().isEmpty(), "the lookup straight after a refusal asked the keychain again, so this test cannot cover the refusal window");
    QCOMPARE(answer->error, CredentialManager::unreadableFileError(mProfile));
}

void CredentialManagerKeychainTest::testAnUndecryptableFileIsReportedWhenTheKeychainStalls_data()
{
    QTest::addColumn<int>("stalledRead");
    QTest::newRow("a read before the file") << 0;
    int readsBeforeTheFile = 0;
    for (const auto& read : expectedReads(QStringLiteral("profile"), QStringLiteral("kctest"))) {
        if (read.name.startsWith(QStringLiteral("colliding"))) {
            break;
        }
        ++readsBeforeTheFile;
    }
    QTest::newRow("a read after the file") << readsBeforeTheFile;
}

// The deadline reads the file too, and has to say what it found there rather than only that it
// gave up waiting
void CredentialManagerKeychainTest::testAnUndecryptableFileIsReportedWhenTheKeychainStalls()
{
    QFETCH(int, stalledRead);
    QVERIFY(storeThenCorrupt(mProfile, mKey));

    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(stalledRead);
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;

    QObject lateContext;
    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY2(staller.waitForAnyStalled(), "the read meant to stall was never started, so this run tested nothing");
    QCOMPARE(answers->first->error, CredentialManager::unreadableFileError(mProfile));
    // still a timeout: the read it gave up on can yet answer with the password
    QVERIFY(answers->timedOut);

    staller.release();
    QTRY_VERIFY2(!staller.firstStalledAlive(), "a read that answered after its lookup gave up was never deleted");
}

// SlySven's report (#11029): a locked or dismissed keychain was asked once per historical layout,
// and each ask is another wallet prompt in front of the player for an answer the store has already
// given. The file is still read - the store has no say over that one - so a password kept there is
// still found.
void CredentialManagerKeychainTest::testAStoreThatRefusesIsNotAskedForEveryOtherLayout()
{
    JobStaller staller;
    staller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QKeychain::Job* firstRead = staller.waitForStalled(0);
    QVERIFY(firstRead);
    JobStaller::answer(firstRead, QKeychain::AccessDenied, QStringLiteral("synthetic: the wallet is locked"));
    QKeychain::Job* secondRead = staller.waitForStalled(1);
    QVERIFY2(secondRead, "one refusal was taken for the whole store, so an entry locked on its own would lose the layouts behind it");
    QVERIFY2(secondRead != firstRead, "the second refusal answered the same read again, so the chain was never followed");
    JobStaller::answer(secondRead, QKeychain::AccessDenied, QStringLiteral("synthetic: the wallet is locked"));

    QVERIFY(waitForAnswer(answer));
    QVERIFY(!answer->success);
    QVERIFY2(answer->error.contains(QStringLiteral("synthetic: the wallet is locked")), qPrintable(QStringLiteral("the refusal was not what the lookup reported: %1").arg(answer->error)));
    // Two reads to tell a locked store from one unreadable entry, and no more: the chain has five or
    // six layouts and each would prompt the player again
    QCOMPARE(staller.reads().size(), 2);
}

// The profile preferences ask about "reconnect" and then "reconnect-token", each through a
// CredentialManager of its own, which is how one dismissed prompt became two rounds of them.
void CredentialManagerKeychainTest::testAFreshRefusalSparesTheNextLookupTheStore()
{
    JobStaller firstStaller;
    firstStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager firstManager;
    firstManager.mJobStartHook = firstStaller.hook();

    const auto firstAnswer = startRetrieval(firstManager, mProfile, mKey);
    // Two, because one refusal could be that entry's own
    for (int refusal = 0; refusal < 2; ++refusal) {
        QKeychain::Job* refused = firstStaller.waitForStalled(refusal);
        QVERIFY(refused);
        JobStaller::answer(refused, QKeychain::AccessDenied, QStringLiteral("synthetic: the wallet is locked"));
    }
    QVERIFY(waitForAnswer(firstAnswer));

    JobStaller secondStaller;
    secondStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager secondManager;
    secondManager.mJobStartHook = secondStaller.hook();

    const auto secondAnswer = startRetrieval(secondManager, mProfile, QStringLiteral("reconnect-token"));
    QVERIFY(waitForAnswer(secondAnswer));
    QVERIFY(!secondAnswer->success);
    QVERIFY2(secondStaller.reads().isEmpty(), "the store was asked again moments after refusing, so the player is prompted twice");
}

// With the profile chosen as the place for passwords, nothing here may reach for the keychain:
// that is what SlySven's setting asks for, and on his desktop every read of it costs a wallet
// prompt. Stored, read back and removed without a single keychain job (#11029).
void CredentialManagerKeychainTest::testTheProfileStoragePreferenceKeepsTheKeychainOutOfIt()
{
    CredentialManager::profileStorageOverrideForTesting() = true;
    const QString secret = QStringLiteral("token-that-stays-in-the-profile");

    JobStaller watcher;
    // Deletes are taken over and answered here rather than reaching the machine's own keychain: the
    // cleanup below is the one keychain job this mode starts, and the test has to see its outcome
    // rather than wait on a real store.
    watcher.stallEvery<QKeychain::DeletePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    bool stored = false;
    bool storeAnswered = false;
    manager.storePassword(mProfile, mKey, secret, [&stored, &storeAnswered](bool success, const QString&) {
        stored = success;
        storeAnswered = true;
    });
    QTRY_VERIFY(storeAnswered);
    QVERIFY2(stored, "the profile's own store did not take the credential");

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QVERIFY2(answer->success, qPrintable(QStringLiteral("the profile's own copy was not read back: %1").arg(answer->error)));
    QCOMPARE(answer->password, secret);

    bool removed = false;
    bool removeAnswered = false;
    manager.removePassword(mProfile, mKey, [&removed, &removeAnswered](bool success, const QString&) {
        removed = success;
        removeAnswered = true;
    });

    // Forgetting reaches across to the keychain, because an entry stored before the preference
    // changed is unreachable by every other path once it points at the file: both names a lookup
    // would read are cleared, and "nothing there" is the ordinary answer for a profile that has only
    // ever used the file.
    // Two entries to clear, and on Windows each is swept twice: removeCredential() follows the
    // named delete with one against the bare key, for entries a qtkeychain before 0.17 wrote under
    // TargetName == key.
    const int expectedDeletions = onWindows() ? 4 : 2;
    for (int deletion = 0; deletion < expectedDeletions; ++deletion) {
        QKeychain::Job* cleanup = watcher.waitForStalled(deletion);
        QVERIFY2(cleanup, "the keychain copy was not cleared, so forgetting a sign-in would leave one behind");
        JobStaller::answer(cleanup, QKeychain::EntryNotFound, QStringLiteral("synthetic: nothing stored here"));
    }

    QTRY_VERIFY(removeAnswered);
    QVERIFY2(removed, "the removal reported failure although both stores are clear");

    QVERIFY2(watcher.reads().isEmpty(), "the keychain was read although the player asked for passwords to be kept in the profile");
    QCOMPARE(watcher.deletes().size(), expectedDeletions);
    // Both layouts a lookup reads, named rather than merely different: clearing the current entry
    // twice under two keys would satisfy "not equal" while leaving the colliding one behind
    QVERIFY2(watcher.deletes().contains({expectedServiceName(mProfile, mKey), expectedServiceName(mProfile, mKey)}),
             qPrintable(QStringLiteral("the current-format entry was not cleared, only: %1").arg(describeDeletes(watcher.deletes()))));
    QVERIFY2(watcher.deletes().contains({expectedLegacyServiceName(mProfile, mKey), expectedLegacyServiceName(mProfile, mKey)}),
             qPrintable(QStringLiteral("the colliding-format entry was not cleared, only: %1").arg(describeDeletes(watcher.deletes()))));
}

// The preference as MudletApp holds it, where either frontend puts Mudlet.ini's value, rather than the
// testing override. Every job is taken over, so none can reach the keychain.
void CredentialManagerKeychainTest::testTheMudletAppPreferenceKeepsTheKeychainOutOfIt()
{
    MudletApp::setStorePasswordsSecurely(false);
    const QString secret = QStringLiteral("token-kept-in-the-profile-with-no-main-window");

    JobStaller watcher;
    watcher.stallEvery<QKeychain::Job>();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    bool stored = false;
    bool storeAnswered = false;
    manager.storePassword(mProfile, mKey, secret, [&stored, &storeAnswered](bool success, const QString&) {
        stored = success;
        storeAnswered = true;
    });
    QTRY_VERIFY2(storeAnswered, "the store never answered, as when it waits on a keychain job");
    QVERIFY2(stored, "the profile's own store did not take the credential");

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QVERIFY2(answer->success, qPrintable(QStringLiteral("the profile's own copy was not read back: %1").arg(answer->error)));
    QCOMPARE(answer->password, secret);

    QCOMPARE(watcher.stalledCount(), 0);
}

// A removal the client makes by itself - a token the game rejected, dropped - can be set off by the
// game as often as it likes, so with the profile chosen it must not reach the keychain: each would
// be a prompt, and a keychain that refused would fail a removal the profile's file had already
// carried out (raised in review of #11032).
void CredentialManagerKeychainTest::testARemovalTheClientMakesItselfLeavesTheKeychainAlone()
{
    CredentialManager::profileStorageOverrideForTesting() = true;
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, QStringLiteral("token-the-game-rejected")));

    JobStaller watcher;
    watcher.stallEvery<QKeychain::ReadPasswordJob>();
    watcher.stallEvery<QKeychain::DeletePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    bool removed = false;
    bool answered = false;
    manager.removePassword(
            mProfile,
            mKey,
            [&removed, &answered](bool success, const QString&) {
                removed = success;
                answered = true;
            },
            CredentialManager::StoreScope::PreferredStore);

    QTRY_VERIFY(answered);
    QVERIFY2(removed, "the removal failed although the profile's own copy went");
    QVERIFY(CredentialManager::retrieveCredential(mProfile, mKey).isEmpty());
    QVERIFY2(watcher.deletes().isEmpty(), qPrintable(QStringLiteral("the keychain was asked to delete: %1").arg(describeDeletes(watcher.deletes()))));
    QVERIFY(watcher.reads().isEmpty());
}

// Forgetting clears the keychain copy an earlier preference left, so the control that offers the
// forget has to see that copy too - or it stays hidden, and the copy is there for good. Only that
// check looks: the lookups a sign-in makes stay with the profile, as the player asked (raised in
// review of #11032).
void CredentialManagerKeychainTest::testTheForgetCheckSeesAKeychainCopyTheProfileLacks()
{
    CredentialManager::profileStorageOverrideForTesting() = true;
    const QString service = expectedServiceName(mProfile, mKey);

    JobStaller watcher;
    watcher.stallNth<QKeychain::ReadPasswordJob>(0);
    watcher.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    std::optional<bool> preferredOnly;
    manager.credentialExists(mProfile, mKey, [&preferredOnly](bool exists) {
        preferredOnly = exists;
    });
    QTRY_VERIFY(preferredOnly.has_value());
    QVERIFY2(watcher.reads().isEmpty(), "a sign-in lookup reached past the profile the player chose");

    std::optional<bool> everyStore;
    manager.credentialExists(
            mProfile,
            mKey,
            [&everyStore](bool exists) {
                everyStore = exists;
            },
            CredentialManager::StoreScope::EveryStore);
    QKeychain::Job* read = watcher.waitForStalled();
    QVERIFY2(read, "the forget check never looked in the keychain for the copy the forget would clear");
    QCOMPARE(read->key(), service);
    JobStaller::answer(read, QKeychain::EntryNotFound, QStringLiteral("synthetic: nothing stored here"));
    QTRY_VERIFY(everyStore.has_value());

    // With the profile's own copy there, that is the answer, and the keychain is not asked at all
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, QStringLiteral("kept-in-the-profile")));
    const auto readsBefore = watcher.reads().size();
    std::optional<bool> fromTheProfile;
    manager.credentialExists(
            mProfile,
            mKey,
            [&fromTheProfile](bool exists) {
                fromTheProfile = exists;
            },
            CredentialManager::StoreScope::EveryStore);
    QTRY_VERIFY(fromTheProfile.has_value());
    QVERIFY(*fromTheProfile);
    QCOMPARE(watcher.reads().size(), readsBefore);
}

// The lookups the window is for: the preferences ask about "reconnect" and then "reconnect-token",
// and a sign-in key's chain has a single keychain read on Linux and macOS - so a refusal that waited
// for a second before counting as the store's never opened the window, and the player was prompted
// for each (raised in review of #11031).
void CredentialManagerKeychainTest::testARefusalOfTheSignInsOnlyKeychainReadSparesTheNextLookup()
{
    JobStaller firstStaller;
    firstStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager firstManager;
    firstManager.mJobStartHook = firstStaller.hook();

    const auto firstAnswer = startRetrieval(firstManager, mProfile, QStringLiteral("reconnect"));
    // Every keychain read the chain has: one, and on Windows the pre-0.17 naming as well
    const int keychainReads = onWindows() ? 2 : 1;
    for (int refusal = 0; refusal < keychainReads; ++refusal) {
        QKeychain::Job* refused = firstStaller.waitForStalled(refusal);
        QVERIFY(refused);
        JobStaller::answer(refused, QKeychain::AccessDenied, QStringLiteral("synthetic: the wallet is locked"));
    }
    QVERIFY(waitForAnswer(firstAnswer));
    QCOMPARE(firstStaller.reads().size(), keychainReads);

    JobStaller secondStaller;
    secondStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager secondManager;
    secondManager.mJobStartHook = secondStaller.hook();

    const auto secondAnswer = startRetrieval(secondManager, mProfile, QStringLiteral("reconnect-token"));
    QVERIFY(waitForAnswer(secondAnswer));
    QVERIFY2(secondStaller.reads().isEmpty(), "the store was asked again moments after refusing, so the player is prompted twice");
}

// A single refusal can be one entry's own - a per-item ACL, or an item another build saved under
// terms this one cannot meet - while the rest of the store reads perfectly well, so the password
// kept in a layout behind it still has to be found. Raised in review of #11031.
void CredentialManagerKeychainTest::testOneUnreadableEntryDoesNotHideAnOlderLayout()
{
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QKeychain::Job* refused = staller.waitForStalled();
    QVERIFY(refused);
    JobStaller::answer(refused, QKeychain::AccessDenied, QStringLiteral("synthetic: this one entry is not readable"));

    QVERIFY(waitForAnswer(answer));
    QCOMPARE(staller.reads().size(), expectedReads(mProfile, mKey).size());
}
// readable, and a lookup for one of those must not be answered out of the file because of a
// refusal that was nothing to do with it (raised in review of #11032).
void CredentialManagerKeychainTest::testARefusalForOneProfileDoesNotSkipTheStoreForAnother()
{
    JobStaller refusingStaller;
    refusingStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager refusedManager;
    refusedManager.mJobStartHook = refusingStaller.hook();

    const auto refusedAnswer = startRetrieval(refusedManager, mProfile, mKey);
    for (int refusal = 0; refusal < 2; ++refusal) {
        QKeychain::Job* read = refusingStaller.waitForStalled(refusal);
        QVERIFY(read);
        JobStaller::answer(read, QKeychain::AccessDenied, QStringLiteral("synthetic: this profile's entries are locked"));
    }
    QVERIFY(waitForAnswer(refusedAnswer));

    JobStaller otherStaller;
    otherStaller.answerOtherReadsNotFound();
    CredentialManager otherManager;
    otherManager.mJobStartHook = otherStaller.hook();

    const QString otherProfile = mProfile + QStringLiteral("-elsewhere");
    const auto otherAnswer = startRetrieval(otherManager, otherProfile, mKey);
    QVERIFY(waitForAnswer(otherAnswer));
    QVERIFY2(!otherStaller.reads().isEmpty(), "another profile's lookup was answered out of the file because this profile's keychain was refused");
}

// What the window is for: while the store is refusing, the file is still read - it is not the
// store's to refuse - so a password kept there goes on working (raised in review of #11032).
void CredentialManagerKeychainTest::testATrimmedLookupStillFindsTheProfilesOwnCopy()
{
    const QString secret = QStringLiteral("kept-in-the-file-while-the-wallet-is-locked");
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, secret));

    JobStaller refusingStaller;
    refusingStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager refusedManager;
    refusedManager.mJobStartHook = refusingStaller.hook();

    const auto refusedAnswer = startRetrieval(refusedManager, mProfile, mKey);
    for (int refusal = 0; refusal < 2; ++refusal) {
        QKeychain::Job* read = refusingStaller.waitForStalled(refusal);
        QVERIFY(read);
        JobStaller::answer(read, QKeychain::AccessDenied, QStringLiteral("synthetic: the wallet is locked"));
    }
    QVERIFY(waitForAnswer(refusedAnswer));
    QVERIFY2(refusedAnswer->success, "the file was not read once the store had refused, so a saved password stops working while a wallet is locked");
    QCOMPARE(refusedAnswer->password, secret);

    JobStaller trimmedStaller;
    trimmedStaller.stallEvery<QKeychain::ReadPasswordJob>();
    CredentialManager trimmedManager;
    trimmedManager.mJobStartHook = trimmedStaller.hook();

    const auto trimmedAnswer = startRetrieval(trimmedManager, mProfile, mKey);
    QVERIFY(waitForAnswer(trimmedAnswer));
    QVERIFY2(trimmedAnswer->success, "the lookup the window trimmed did not read the file, so the password it holds was lost");
    QCOMPARE(trimmedAnswer->password, secret);
    QVERIFY2(trimmedStaller.reads().isEmpty(), "the store was asked again moments after refusing");

    CredentialManager::removeCredential(mProfile, mKey);
}

// Forgetting has to be honest: a keychain that holds a credential and will not let it go means the
// sign-in is not gone, whatever the profile's own copy did (raised in review of #11032).
void CredentialManagerKeychainTest::testForgettingSaysSoWhenTheKeychainCopyRemains()
{
    CredentialManager::profileStorageOverrideForTesting() = true;
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, QStringLiteral("a-token-in-both-stores")));

    JobStaller watcher;
    watcher.stallEvery<QKeychain::DeletePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    bool removed = true;
    bool removeAnswered = false;
    QString reported;
    manager.removePassword(mProfile, mKey, [&removed, &removeAnswered, &reported](bool success, const QString& error) {
        removed = success;
        reported = error;
        removeAnswered = true;
    });

    const int deletions = onWindows() ? 4 : 2;
    for (int deletion = 0; deletion < deletions; ++deletion) {
        QKeychain::Job* cleanup = watcher.waitForStalled(deletion);
        QVERIFY(cleanup);
        // The first refuses; the rest answer, so the refusal is the only reason to report failure
        JobStaller::answer(cleanup,
                           deletion == 0 ? QKeychain::AccessDenied : QKeychain::EntryNotFound,
                           deletion == 0 ? QStringLiteral("synthetic: this entry will not be deleted") : QStringLiteral("synthetic: nothing stored here"));
    }

    QTRY_VERIFY(removeAnswered);
    QVERIFY2(!removed, "a keychain copy that could not be removed was reported as forgotten");
    QVERIFY2(reported.contains(QStringLiteral("keychain")), qPrintable(QStringLiteral("the failure does not say where the credential still is: %1").arg(reported)));
    QVERIFY2(reported.contains(QStringLiteral("synthetic: this entry will not be deleted")), qPrintable(QStringLiteral("the keychain's own reason was dropped: %1").arg(reported)));
}

// With no Secret Service on D-Bus, libsecret answers every job with OtherError. A store that fails
// falls back to the encrypted file, so nothing can be in a keychain that isn't there, and a removal
// should not report one as left behind (raised in review of #11906).
void CredentialManagerKeychainTest::testAKeychainNotOnDBusHoldsNothingToRemove()
{
    JobStaller watcher;
    watcher.stallEvery<QKeychain::DeletePasswordJob>();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();

    bool removed = false;
    bool removeAnswered = false;
    manager.removePassword(mProfile, mKey, [&removed, &removeAnswered](bool success, const QString&) {
        removed = success;
        removeAnswered = true;
    });

    const int deletions = onWindows() ? 4 : 2;
    for (int deletion = 0; deletion < deletions; ++deletion) {
        QKeychain::Job* cleanup = watcher.waitForStalled(deletion);
        QVERIFY(cleanup);
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not reach the keychain.*org\\.freedesktop\\.secrets")));
#endif
        JobStaller::answer(cleanup, QKeychain::OtherError, QStringLiteral("The name org.freedesktop.secrets was not provided by any .service files"));
    }

    QTRY_VERIFY(removeAnswered);
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QVERIFY2(!removed, "a keychain that is part of the OS failing to delete an entry was reported as nothing left behind");
#else
    QVERIFY2(removed, "a keychain with no service on D-Bus was reported as still holding the credential");
#endif
}

// Once the store has answered a read it is unlocked, so every refusal after that is one entry's
// own however many of them arrive - and the password may be in a layout behind them. Two in a row
// must not be read as a locked store here, which is what separates "nothing has answered" from
// "this entry will not answer" (raised in review of #11031).
void CredentialManagerKeychainTest::testRefusalsAfterTheStoreHasAnsweredDoNotStopTheChain()
{
    JobStaller staller;
    // The first read answers, the two after it refuse
    staller.stallNths<QKeychain::ReadPasswordJob>({1, 2});
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, mKey);
    for (int refusal = 0; refusal < 2; ++refusal) {
        QKeychain::Job* refused = staller.waitForStalled(refusal);
        QVERIFY(refused);
        JobStaller::answer(refused, QKeychain::AccessDenied, QStringLiteral("synthetic: this entry will not answer"));
    }

    QVERIFY(waitForAnswer(answer));
    QVERIFY2(staller.reads().size() == expectedReads(mProfile, mKey).size(),
             qPrintable(QStringLiteral("the chain stopped after two refusals although the store had already answered: %1 of %2 layouts were read")
                                .arg(staller.reads().size())
                                .arg(expectedReads(mProfile, mKey).size())));
}

// A reconnect token cannot be in the colliding format - that layout belongs to 4.20.0 and 4.20.1,
// months before the sign-in keys existed - nor in the key=account layout that the Windows keychain
// fix replaced. Reading them anyway found nothing and cost the player a prompt each, on a keychain
// that asks per item: two of them per profile, on the two keys the profile preferences ask about
// (#11029, reported against a development build).
void CredentialManagerKeychainTest::testASignInKeyIsNotLookedForInLayoutsOlderThanItself()
{
    JobStaller staller;
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();

    const auto answer = startRetrieval(manager, mProfile, QStringLiteral("reconnect-token"));
    QVERIFY(waitForAnswer(answer));

    const QString service = expectedServiceName(mProfile, QStringLiteral("reconnect-token"));
    QList<QPair<QString, QString>> expected;
    expected.append({service, service});
#if defined(Q_OS_WIN)
    // The bare-name read stays: it is the same entry under the naming a qtkeychain before 0.17 used,
    // not an older layout of Mudlet's own
    expected.append({QString(), service});
#endif
    QCOMPARE(staller.reads(), expected);
}

// A caller that would remove what it cannot read back - the sign-in's rotated-token check - has to
// tell "the store has nothing" from "the store would not say"
void CredentialManagerKeychainTest::testALookupThatFindsNothingSaysWhetherTheStoreAnswered_data()
{
    QTest::addColumn<QKeychain::Error>("firstReadError");
    QTest::addColumn<bool>("garbledFile");
    QTest::addColumn<CredentialManager::ReadOutcome>("expected");
    QTest::newRow("nothing stored anywhere") << QKeychain::NoError << false << CredentialManager::ReadOutcome::NothingStored;
    QTest::newRow("the first read refused") << QKeychain::AccessDenied << false << CredentialManager::ReadOutcome::Unreadable;
    // What libsecret reports when no Secret Service is running on D-Bus
    QTest::newRow("no keychain service to ask") << QKeychain::OtherError << false << CredentialManager::ReadOutcome::Unavailable;
    QTest::newRow("the encrypted file garbled") << QKeychain::NoError << true << CredentialManager::ReadOutcome::Unreadable;
    QTest::newRow("no keychain service, and the file garbled") << QKeychain::OtherError << true << CredentialManager::ReadOutcome::Unreadable;
}

void CredentialManagerKeychainTest::testALookupThatFindsNothingSaysWhetherTheStoreAnswered()
{
    QFETCH(QKeychain::Error, firstReadError);
    QFETCH(bool, garbledFile);
    QFETCH(CredentialManager::ReadOutcome, expected);
    const QString filePath = CredentialManager::generateFilePath(mProfile, mKey);
    const auto removeFile = qScopeGuard([&filePath] {
        QFile::remove(filePath);
    });
    if (garbledFile) {
        QVERIFY(QDir().mkpath(QFileInfo(filePath).absolutePath()));
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("not an encrypted credential");
    }
    JobStaller staller;
    if (firstReadError != QKeychain::NoError) {
        staller.stallNth<QKeychain::ReadPasswordJob>(0);
    }
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    QObject lateContext;

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    if (firstReadError != QKeychain::NoError) {
        QKeychain::Job* firstRead = staller.waitForStalled();
        QVERIFY(firstRead);
        JobStaller::answer(firstRead, firstReadError, QStringLiteral("synthetic: the first read failed"));
    }
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY(!answers->first->success);
    QCOMPARE(answers->outcome, expected);
}

void CredentialManagerKeychainTest::testALookupThatAnswersInTimeOwesNoLateAnswer()
{
    JobStaller recorder;
    recorder.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = recorder.hook();
    // Long enough for the staller to answer every read, which it does a posted event at a time
    manager.mOperationTimeoutMs = 1000;
    QObject lateContext;

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY(!answers->first->success);
    QVERIFY2(!answers->timedOut, "a lookup that went all the way down its chain was reported as timed out");
    // Past where the deadline would have fallen
    QTest::qWait(1500ms);
    QCOMPARE(answers->first->count, 1);
    QCOMPARE(answers->late->count, 0);
}

void CredentialManagerKeychainTest::testALateAnswerFollowsALookupThatTimedOut_data()
{
    QTest::addColumn<QKeychain::Error>("lateError");
    QTest::addColumn<bool>("refused");
    // The user denying the prompt, which has to be told apart from a prompt nobody answers
    QTest::newRow("access refused") << QKeychain::AccessDenied << true;
    QTest::newRow("nothing there") << QKeychain::EntryNotFound << false;
}

void CredentialManagerKeychainTest::testALateAnswerFollowsALookupThatTimedOut()
{
    QFETCH(QKeychain::Error, lateError);
    QFETCH(bool, refused);
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;
    QObject lateContext;

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    QVERIFY2(waitForAnswer(answers->first), "a lookup whose keychain read never answers must still answer its caller");
    QKeychain::Job* read = staller.waitForStalled();
    QVERIFY2(read, "the read meant to stall was never started, so this run tested nothing");
    QVERIFY(!answers->first->success);
    QVERIFY2(answers->timedOut, "a lookup that gave up waiting on the keychain did not say so");
    QCOMPARE(answers->first->error, QStringLiteral("Operation timed out"));
    QCOMPARE(answers->late->count, 0);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("after its lookup had timed out, without the password")));
    JobStaller::answer(read, lateError, QStringLiteral("synthetic: answered at last"));
    QVERIFY2(waitForAnswer(answers->late), "the keychain's answer to a read the lookup had given up on never reached the caller");
    QVERIFY(!answers->late->success);
    QCOMPARE(answers->late->error,
             refused ? QStringLiteral("Could not read the keychain: synthetic: answered at last") : QStringLiteral("No password in the current format for profile %1").arg(mProfile));
    QTest::qWait(100ms);
    QCOMPARE(answers->first->count, 1);
    QCOMPARE(answers->late->count, 1);
}

void CredentialManagerKeychainTest::testALateAnswerIsDroppedOnceWhatAskedForItHasGone()
{
    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;
    auto lateContext = std::make_unique<QObject>();

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, lateContext.get());
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY(answers->timedOut);
    QKeychain::Job* read = staller.waitForStalled();
    QVERIFY(read);

    // As the profile or dialog that asked does when it closes before the keychain answers
    lateContext.reset();
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("after its lookup had timed out, but whatever asked for it has gone")));
    JobStaller::answer(read, QKeychain::AccessDenied, QStringLiteral("synthetic: answered at last"));
    QTest::qWait(100ms);
    QCOMPARE(answers->late->count, 0);
}

void CredentialManagerKeychainTest::testALateAnswerHandsOverThePassword()
{
    if (!mStoreAvailable) {
        QSKIP("credential store unavailable in this environment");
    }
    const QString service = expectedServiceName(mProfile, mKey);
    const QString secret = QStringLiteral("late-secret");
    QVERIFY(writeEntry(service, service, secret));

    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;
    QObject lateContext;

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY(answers->timedOut);
    QKeychain::Job* read = staller.waitForStalled();
    QVERIFY(read);
    QCOMPARE(read->key(), service);

    // The user answering the prompt at last: the read reaches the store, which has the entry
    staller.forget(read);
    read->start();
    const bool lateAnswered = waitForAnswer(answers->late);
    deleteEntry(service, service);

    QVERIFY2(lateAnswered, "the password the keychain handed over after the lookup had given up never reached the caller");
    QVERIFY(answers->late->success);
    QCOMPARE(answers->late->password, secret);
    QCOMPARE(answers->first->count, 1);
}

// A keychain write leaves the file as it was, so the copy read there at the deadline can be an
// older password than the one the stalled read is about to hand over
void CredentialManagerKeychainTest::testALateAnswerFollowsTheFilesOlderCopy()
{
    if (!mStoreAvailable) {
        QSKIP("credential store unavailable in this environment");
    }
    const QString service = expectedServiceName(mProfile, mKey);
    const QString older = QStringLiteral("older-file-secret");
    const QString newer = QStringLiteral("newer-keychain-secret");
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, older));
    QVERIFY(writeEntry(service, service, newer));

    JobStaller staller;
    staller.stallNth<QKeychain::ReadPasswordJob>(0);
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;
    QObject lateContext;

    const auto answers = startTimedRetrieval(manager, mProfile, mKey, &lateContext);
    const bool answered = waitForAnswer(answers->first);
    QKeychain::Job* read = staller.waitForStalled();
    bool lateAnswered = false;
    if (read) {
        staller.forget(read);
        read->start();
        lateAnswered = waitForAnswer(answers->late);
    }
    CredentialManager::removeCredential(mProfile, mKey);
    deleteEntry(service, service);

    QVERIFY(answered);
    QVERIFY2(read, "the read meant to stall was never started, so this run tested nothing");
    QVERIFY2(answers->first->success, qPrintable(answers->first->error));
    QCOMPARE(answers->first->password, older);
    QVERIFY2(lateAnswered, "the keychain's newer password never followed the file's older one");
    QVERIFY(answers->late->success);
    QCOMPARE(answers->late->password, newer);
    QCOMPARE(answers->first->count, 1);
    QCOMPARE(answers->late->count, 1);
}

// What an Android app's chosen root does with the KeyStore, which every root the app names shares
void CredentialManagerKeychainTest::testAChosenRootsKeychainEntriesAreItsOwn()
{
    QTemporaryDir first;
    QTemporaryDir second;
    QVERIFY(first.isValid() && second.isValid());
    QStringList othersEntries{expectedServiceName(mProfile, mKey)};

    for (const QTemporaryDir* root : {&first, &second}) {
        MudletApp::chooseConfigRoot(root->path(), false);
        const QString entry = expectedServiceName(mProfile, mKey, root->path());
        JobStaller watcher;
        watcher.answerOtherReadsNotFound();
        CredentialManager manager;
        manager.mJobStartHook = watcher.hook();
        QVERIFY(waitForAnswer(startRetrieval(manager, mProfile, mKey)));

        QVERIFY2(!watcher.reads().isEmpty(), "the keychain was never asked, so this run tested nothing");
        QCOMPARE(watcher.reads().constFirst().first, entry);
        for (const auto& read : watcher.reads()) {
            QVERIFY2(!othersEntries.contains(read.first), "a new profile here would remove the password another root keeps for the same name");
        }

        // As THeadlessSession's removeStalePasswords() does for a profile new to this root
        const auto removed = forgetAnsweringDeletes(manager, watcher, mProfile, mKey);
        QVERIFY(waitForAnswer(removed));
        QVERIFY2(removed->success, qPrintable(removed->error));
        QVERIFY2(watcher.deletes().contains(qMakePair(entry, entry)), qPrintable(describeDeletes(watcher.deletes())));
        // The colliding format's name is the same for every root, but no Android build ever wrote that layout
        for (const auto& removal : watcher.deletes()) {
            QVERIFY2(!othersEntries.contains(removal.first), "a new profile here removed the password another root keeps for the same name");
        }
        othersEntries << entry;
    }
}

// A second name for the folder, as a symlink gives
void CredentialManagerKeychainTest::testAChosenRootsEntriesAreNamedForTheFolderNotItsSpelling()
{
#if defined(Q_OS_WIN)
    QSKIP("QFile::link() makes a shortcut on Windows, not a second name for the folder");
#endif
    QTemporaryDir parent;
    QVERIFY(parent.isValid());
    const QString folder = parent.filePath(QStringLiteral("files"));
    const QString alias = parent.filePath(QStringLiteral("alias"));
    QVERIFY(QDir().mkpath(folder));
    QVERIFY(QFile::link(folder, alias));

    // So the passwords saved under one spelling of the folder are found under the other
    const QString expected = expectedServiceName(mProfile, mKey, folder);
    for (const QString& spelling : {folder, alias}) {
        MudletApp::chooseConfigRoot(spelling, false);
        JobStaller watcher;
        watcher.answerOtherReadsNotFound();
        CredentialManager manager;
        manager.mJobStartHook = watcher.hook();
        QVERIFY(waitForAnswer(startRetrieval(manager, mProfile, mKey)));
        QVERIFY2(!watcher.reads().isEmpty(), "the keychain was never asked, so this run tested nothing");
        QCOMPARE(watcher.reads().constFirst().first, expected);
    }
}

void CredentialManagerKeychainTest::testAChosenRootReadsTheSaveItsKeychainRefusedFirst()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);

    JobStaller staller;
    staller.stallEvery<QKeychain::WritePasswordJob>();
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    const auto stored = startStore(manager, mProfile, mKey, QStringLiteral("refused-secret"));
    QKeychain::Job* store = staller.waitForStalled();
    QVERIFY2(store, "the store never reached the keychain, so this run tested nothing");
    JobStaller::answer(store, QKeychain::AccessDenied, QStringLiteral("synthetic: the KeyStore refused the write"));
    QVERIFY(waitForAnswer(stored));
    QVERIFY2(stored->success, qPrintable(stored->error));
    QVERIFY(!manager.keychainRefusal().isEmpty());
    QVERIFY2(QDir(root.filePath(QStringLiteral("credentials/profiles"))).exists(), "the password the keychain refused was not kept under the chosen root");

    const auto answer = startRetrieval(manager, mProfile, mKey);
    QVERIFY(waitForAnswer(answer));
    QCOMPARE(answer->password, QStringLiteral("refused-secret"));
    QVERIFY2(staller.reads().isEmpty(), "the keychain was read first, where an older copy would hide the password it refused");
}

void CredentialManagerKeychainTest::testAnUnreadableFileUnderAChosenRootIsNotPassedOverForTheKeychain_data()
{
    QTest::addColumn<HiddenCopy>("copy");
    QTest::newRow("a file it cannot decrypt") << HiddenCopy::CannotDecrypt;
    QTest::newRow("a file its folder hides") << HiddenCopy::FolderHidesIt;
    QTest::newRow("a file that cannot be looked at") << HiddenCopy::CannotBeLookedAt;
}

void CredentialManagerKeychainTest::testAnUnreadableFileUnderAChosenRootIsNotPassedOverForTheKeychain()
{
    QFETCH(HiddenCopy, copy);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);
    QVERIFY(storeThenCorrupt(mProfile, mKey));
    const QString path = CredentialManager::generateFilePath(mProfile, mKey);
    const QString folder = QFileInfo(path).absolutePath();
    const auto reopen = qScopeGuard([&folder]() {
        QFile::setPermissions(folder, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });
    switch (copy) {
    case HiddenCopy::CannotDecrypt:
        break;
    case HiddenCopy::FolderHidesIt:
        QVERIFY(QFile::setPermissions(folder, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        if (QFileInfo::exists(path)) {
            QSKIP("folder permissions cannot hide a file here (Windows, or running as root)");
        }
        break;
    case HiddenCopy::CannotBeLookedAt:
#if defined(Q_OS_WIN)
        QSKIP("QFile::link() makes a shortcut on Windows, not a link stat() follows");
#endif
        // A link to itself, which stat() fails on with ELOOP
        QVERIFY(QFile::remove(path) && QFile::link(path, path));
        break;
    case HiddenCopy::OlderNamedThatStays:
        QFAIL("not a row of this test");
    }

    JobStaller watcher;
    watcher.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();
    const auto answers = startTimedRetrieval(manager, mProfile, mKey, nullptr);
    QVERIFY(waitForAnswer(answers->first));
    QCOMPARE(answers->outcome, CredentialManager::ReadOutcome::Unreadable);
    QVERIFY2(watcher.reads().isEmpty(), "the keychain was read past an unreadable file, which is read before the keychain, so an older copy the keychain holds would be the password sent");
}

void CredentialManagerKeychainTest::testOnlyAnOlderNamedCopyTheProfileDecryptsAnswersFirstUnderAChosenRoot_data()
{
    QTest::addColumn<bool>("decrypts");
    QTest::newRow("a copy the profile decrypts") << true;
    QTest::newRow("a copy that will not open") << false;
}

// Only a copy the profile decrypts is provably its own, so only that one can be removed by a save or a forget
void CredentialManagerKeychainTest::testOnlyAnOlderNamedCopyTheProfileDecryptsAnswersFirstUnderAChosenRoot()
{
    QFETCH(bool, decrypts);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);
    // The older name cut a profile's name to its first 50 characters
    const QString profile = QString(50, QLatin1Char('u')) + mProfile;
    const QString legacyPath = CredentialManager::generateLegacyFilePath(profile, mKey);
    QVERIFY(!legacyPath.isEmpty());
    if (decrypts) {
        QVERIFY(writeEncryptedCopy(legacyPath, profile, QStringLiteral("older-named-secret")));
    } else {
        // QFile cannot open a folder, even for root
        QVERIFY(QDir().mkpath(legacyPath + QStringLiteral("/kept")));
    }

    JobStaller watcher;
    watcher.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();
    const auto answer = startRetrieval(manager, profile, mKey);
    QVERIFY(waitForAnswer(answer));
    if (decrypts) {
        QCOMPARE(answer->password, QStringLiteral("older-named-secret"));
        QVERIFY2(watcher.reads().isEmpty(), "the keychain was read before the profile's own copy in the file, which is read first");
    } else {
        QVERIFY2(!watcher.reads().isEmpty(), "a copy nothing can remove kept the keychain from being read, so no save or forget could ever reach it");
    }
}

void CredentialManagerKeychainTest::testUnderAChosenRootTheFileCopyGoesOnlyOnceTheKeychainGivesTheSaveBack_data()
{
    QTest::addColumn<bool>("chosenRoot");
    QTest::addColumn<QKeychain::Error>("readBackError");
    QTest::addColumn<QString>("readBack");
    QTest::newRow("the default root") << false << QKeychain::NoError << QString();
    QTest::newRow("a read gives the save back") << true << QKeychain::NoError << QStringLiteral("newer-secret");
    QTest::newRow("a read gives back something else") << true << QKeychain::NoError << QStringLiteral("older-secret");
    QTest::newRow("a read finds nothing") << true << QKeychain::EntryNotFound << QString();
}

// A write answered with NoError is all QtKeychain's Android backend gives even when its settings file never took it
void CredentialManagerKeychainTest::testUnderAChosenRootTheFileCopyGoesOnlyOnceTheKeychainGivesTheSaveBack()
{
    QFETCH(bool, chosenRoot);
    QFETCH(QKeychain::Error, readBackError);
    QFETCH(QString, readBack);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    if (chosenRoot) {
        MudletApp::chooseConfigRoot(root.path(), false);
    }
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, QStringLiteral("older-secret")));

    JobStaller staller;
    staller.stallEvery<QKeychain::WritePasswordJob>();
    staller.answerOtherReadsNotFound(readBackError);
    CredentialManager::readTextForTesting() = readBack;
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    const auto stored = startStore(manager, mProfile, mKey, QStringLiteral("newer-secret"));
    QKeychain::Job* store = staller.waitForStalled();
    QVERIFY2(store, "the store never reached the keychain, so this run tested nothing");
    store->emitFinished();
    QVERIFY(waitForAnswer(stored));
    QVERIFY2(stored->success, qPrintable(stored->error));
    const QString inTheFile = CredentialManager::retrieveCredential(mProfile, mKey);
    if (!chosenRoot) {
        // The default root reads the keychain first, and leaves the file copy as it always has
        QCOMPARE(inTheFile, QStringLiteral("older-secret"));
        QVERIFY2(staller.reads().isEmpty(), "the default root read a save back");
    } else {
        const QString entry = expectedServiceName(mProfile, mKey, root.path());
        QCOMPARE(staller.reads(), (QList<QPair<QString, QString>>{{entry, entry}}));
        if (readBack == QStringLiteral("newer-secret")) {
            QVERIFY2(inTheFile.isEmpty(), "the older copy is still in the file, which answers before the keychain");
            QVERIFY(manager.keychainRefusal().isEmpty());
        } else {
            QVERIFY2(inTheFile == QStringLiteral("newer-secret"), "the keychain did not give the save back, yet the file copy, the one read, does not hold it");
            QVERIFY(!manager.keychainRefusal().isEmpty());
        }
    }
}

void CredentialManagerKeychainTest::testUnderAChosenRootNeitherASaveNorAForgetSucceedsWhileACopyIsStillRead_data()
{
    QTest::addColumn<Settling>("settling");
    QTest::addColumn<HiddenCopy>("copy");
    const std::pair<Settling, const char*> settlings[] = {{Settling::Save, "a save"}, {Settling::Forget, "a forget"}, {Settling::ForgetPreferredFilesAlone, "a forget of the preferred files alone"}};
    const std::pair<HiddenCopy, const char*> copies[] = {{HiddenCopy::OlderNamedThatStays, "an older-named copy that will not go"},
                                                         {HiddenCopy::FolderHidesIt, "a folder that hides the file"},
                                                         {HiddenCopy::CannotBeLookedAt, "a file that cannot be looked at"}};
    for (const auto& [settling, settlingName] : settlings) {
        for (const auto& [copy, copyName] : copies) {
            QTest::addRow("%s, %s", settlingName, copyName) << settling << copy;
        }
    }
}

void CredentialManagerKeychainTest::testUnderAChosenRootNeitherASaveNorAForgetSucceedsWhileACopyIsStillRead()
{
    QFETCH(Settling, settling);
    QFETCH(HiddenCopy, copy);
    const bool olderNamed = copy == HiddenCopy::OlderNamedThatStays;
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);
    // The older name cut a profile's name to its first 50 characters
    const QString profile = QString(50, QLatin1Char('u')) + mProfile;
    const QString path = olderNamed ? CredentialManager::generateLegacyFilePath(profile, mKey) : CredentialManager::generateFilePath(profile, mKey);
    const QString folder = QFileInfo(path).absolutePath();
    const auto reopen = qScopeGuard([&folder]() {
#if !defined(Q_OS_WIN)
        [[maybe_unused]] const int handedBack = ::chown(QFile::encodeName(folder).constData(), ::geteuid(), ::getegid());
#endif
        QFile::setPermissions(folder, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });
    switch (copy) {
    case HiddenCopy::FolderHidesIt:
        QVERIFY(writeEncryptedCopy(path, profile, QStringLiteral("older-secret")));
        QVERIFY(QFile::setPermissions(folder, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        if (QFileInfo::exists(path)) {
            QSKIP("folder permissions bind nobody here (Windows, or running as root)");
        }
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("the folder %1 does not let Mudlet look inside it").arg(QRegularExpression::escape(folder))));
        break;
    case HiddenCopy::OlderNamedThatStays: {
#if defined(Q_OS_WIN)
        QSKIP("needs a folder Mudlet does not own");
#else
        QVERIFY(writeEncryptedCopy(path, profile, QStringLiteral("older-secret")));
        // Mudlet opens a folder it owns up again before removing the copy, so only one it does not own keeps it
        const auto openToRead = QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther;
        QVERIFY(QFile::setPermissions(folder, openToRead));
        if (::chown(QFile::encodeName(folder).constData(), 65534, 65534) != 0 || QFileInfo(folder).isWritable() || QFile::setPermissions(folder, openToRead)) {
            if (qEnvironmentVariableIsSet("MUDLET_TEST_REQUIRE_UNPRIVILEGED_ROOT")) {
                QFAIL("MUDLET_TEST_REQUIRE_UNPRIVILEGED_ROOT is set, but the folder was not put out of Mudlet's reach (needs root without CAP_DAC_OVERRIDE and CAP_FOWNER)");
            }
            QSKIP("needs root without CAP_DAC_OVERRIDE and CAP_FOWNER, to hand the folder to another user");
        }
#endif
        break;
    }
    case HiddenCopy::CannotBeLookedAt:
#if defined(Q_OS_WIN)
        QSKIP("QFile::link() makes a shortcut on Windows, not a link stat() follows");
#endif
        // A link to itself, which stat() fails on with ELOOP
        QVERIFY(QDir().mkpath(folder) && QFile::link(path, path));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("looking at %1 failed").arg(QRegularExpression::escape(path))));
        break;
    case HiddenCopy::CannotDecrypt:
        QFAIL("not a row of this test");
    }

    JobStaller staller;
    staller.answerOtherReadsNotFound(QKeychain::NoError);
    CredentialManager::readTextForTesting() = QStringLiteral("newer-secret");
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    std::shared_ptr<Answer> answer;
    switch (settling) {
    case Settling::Save: {
        staller.stallEvery<QKeychain::WritePasswordJob>();
        answer = startStore(manager, profile, mKey, QStringLiteral("newer-secret"));
        QKeychain::Job* store = staller.waitForStalled();
        QVERIFY2(store, "the store never reached the keychain, so this run tested nothing");
        store->emitFinished();
        break;
    }
    case Settling::Forget:
    case Settling::ForgetPreferredFilesAlone: {
        // Only this scope leaves the answer to the files' own removal: every store's forget also asks the keychain,
        // whose answer checks the files again
        const bool filesAlone = settling == Settling::ForgetPreferredFilesAlone;
        CredentialManager::profileStorageOverrideForTesting() = filesAlone;
        answer = forgetAnsweringDeletes(manager, staller, profile, mKey, filesAlone ? CredentialManager::StoreScope::PreferredStore : CredentialManager::StoreScope::EveryStore);
        break;
    }
    }
    QVERIFY(waitForAnswer(answer));
    QVERIFY2(!answer->success, "reported done while a copy that is read in place of the keychain stays");
}

void CredentialManagerKeychainTest::testUnderAChosenRootAnAnswerAfterTheTimeoutLeavesTheFileCopy_data()
{
    QTest::addColumn<bool>("readBackLate");
    QTest::addColumn<bool>("taken");
    QTest::newRow("the write, taken") << false << true;
    QTest::newRow("the write, refused") << false << false;
    QTest::newRow("the read back, giving the save back") << true << true;
}

// The save was reported as failed, so an answer nobody waits for proves nothing and the file copy, read first, stays
void CredentialManagerKeychainTest::testUnderAChosenRootAnAnswerAfterTheTimeoutLeavesTheFileCopy()
{
    QFETCH(bool, readBackLate);
    QFETCH(bool, taken);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);
    QVERIFY(CredentialManager::storeCredential(mProfile, mKey, QStringLiteral("older-secret")));

    JobStaller staller;
    if (readBackLate) {
        staller.stallEvery<QKeychain::Job>();
    } else {
        staller.stallEvery<QKeychain::WritePasswordJob>();
        staller.answerOtherReadsNotFound(QKeychain::NoError);
    }
    CredentialManager::readTextForTesting() = QStringLiteral("newer-secret");
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    manager.mOperationTimeoutMs = 300;
    const auto stored = startStore(manager, mProfile, mKey, QStringLiteral("newer-secret"));
    QPointer<QKeychain::Job> late = staller.waitForStalled(0);
    QVERIFY2(late, "the store never reached the keychain, so this run tested nothing");
    if (readBackLate) {
        late->emitFinished();
        late = staller.waitForStalled(1);
        QVERIFY2(late, "the save was never read back, so this run tested nothing");
    }
    QVERIFY(waitForAnswer(stored));
    QVERIFY2(!stored->success, "the save was answered before the keychain had");

    // The manager's next operation is not this save's to answer
    const auto next = startStore(manager, mProfile, mKey + QStringLiteral("-next"), QStringLiteral("next-secret"));
    QVERIFY(staller.waitForStalled(readBackLate ? 2 : 1));
    if (taken) {
        late->emitFinished();
    } else {
        JobStaller::answer(late, QKeychain::AccessDenied, QStringLiteral("synthetic: the KeyStore refused the write late"));
    }
    QTRY_VERIFY(!late);
    QCoreApplication::processEvents();
    QVERIFY2(CredentialManager::retrieveCredential(mProfile, mKey) == QStringLiteral("older-secret"),
             "an answer that came after the save had timed out changed the file copy, which is read before the keychain");
    QCOMPARE(next->count, 0);
}

// QtKeychain's Android backend keeps each entry in QSettings(service), and passes over that file without an error when
// it cannot be read: a read answers EntryNotFound and a delete NoError
void CredentialManagerKeychainTest::testUnderAChosenRootAKeychainStoreThatWillNotOpenIsNotTakenForAnEmptyOne_data()
{
    QTest::addColumn<bool>("forget");
    QTest::addColumn<bool>("unreadable");
    QTest::newRow("a read, no store file") << false << false;
    QTest::newRow("a read, a store file Mudlet cannot read") << false << true;
    QTest::newRow("a forget, no store file") << true << false;
    QTest::newRow("a forget, a store file Mudlet cannot read") << true << true;
}

void CredentialManagerKeychainTest::testUnderAChosenRootAKeychainStoreThatWillNotOpenIsNotTakenForAnEmptyOne()
{
#if !defined(Q_OS_LINUX)
    QSKIP("QSettings keeps its native format in an INI file, as it does on Android, only on Linux");
#else
    QFETCH(bool, forget);
    QFETCH(bool, unreadable);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), false);
    const QString storePath = QSettings(expectedServiceName(mProfile, mKey, root.path())).fileName();
    const auto removeStore = qScopeGuard([&storePath]() {
        QFile::setPermissions(storePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QFile::remove(storePath);
    });
    if (unreadable) {
        QVERIFY(QDir().mkpath(QFileInfo(storePath).absolutePath()));
        QFile store(storePath);
        QVERIFY(store.open(QIODevice::WriteOnly));
        QVERIFY(store.write("[General]\nentry=1\n") > 0);
        store.close();
        QVERIFY(QFile::setPermissions(storePath, QFileDevice::Permissions()));
        if (QFile(storePath).open(QIODevice::ReadOnly)) {
            if (qEnvironmentVariableIsSet("MUDLET_TEST_REQUIRE_UNPRIVILEGED_ROOT")) {
                QFAIL("MUDLET_TEST_REQUIRE_UNPRIVILEGED_ROOT is set, but a file denied to its owner could still be read");
            }
            QSKIP("needs root without CAP_DAC_OVERRIDE and CAP_DAC_READ_SEARCH, or another user, for a file Mudlet cannot read");
        }
    }

    JobStaller staller;
    staller.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = staller.hook();
    if (forget) {
        const auto answer = forgetAnsweringDeletes(manager, staller, mProfile, mKey, CredentialManager::StoreScope::EveryStore, QKeychain::NoError);
        QVERIFY(waitForAnswer(answer));
        QVERIFY2(!staller.deletes().isEmpty(), "the keychain was never asked, so this run tested nothing");
        if (unreadable) {
            QVERIFY2(!answer->success, "a forget was reported done while the keychain's store, which it could not open, may still hold the entry");
            QVERIFY2(answer->error.contains(storePath), qPrintable(answer->error));
        } else {
            QVERIFY2(answer->success, qPrintable(answer->error));
        }
        return;
    }
    const auto answers = startTimedRetrieval(manager, mProfile, mKey, nullptr);
    QVERIFY(waitForAnswer(answers->first));
    QVERIFY2(!staller.reads().isEmpty(), "the keychain was never asked, so this run tested nothing");
    if (unreadable) {
        QVERIFY2(answers->outcome == CredentialManager::ReadOutcome::Unavailable,
                 qPrintable(QStringLiteral("a keychain store that would not open was taken for one that holds nothing: %1").arg(answers->first->error)));
        QVERIFY2(answers->first->error.contains(storePath), qPrintable(answers->first->error));
    } else {
        QCOMPARE(answers->outcome, CredentialManager::ReadOutcome::NothingStored);
    }
#endif
}

void CredentialManagerKeychainTest::testTheForgetCheckUnderAChosenRootAsksTheKeychainOnlyWhenItServesIt_data()
{
    QTest::addColumn<bool>("keepsPasswords");
    QTest::newRow("a root that keeps its passwords") << true;
    QTest::newRow("an Android app's root") << false;
}

void CredentialManagerKeychainTest::testTheForgetCheckUnderAChosenRootAsksTheKeychainOnlyWhenItServesIt()
{
    QFETCH(bool, keepsPasswords);
    QTemporaryDir root;
    QVERIFY(root.isValid());
    MudletApp::chooseConfigRoot(root.path(), keepsPasswords);
    CredentialManager::profileStorageOverrideForTesting() = true;

    JobStaller watcher;
    watcher.answerOtherReadsNotFound();
    CredentialManager manager;
    manager.mJobStartHook = watcher.hook();
    std::optional<bool> exists;
    manager.credentialExists(
            mProfile,
            mKey,
            [&exists](bool found) {
                exists = found;
            },
            CredentialManager::StoreScope::EveryStore);
    QTRY_VERIFY(exists.has_value());
    QCOMPARE(watcher.reads().isEmpty(), keepsPasswords);
}

MUDLET_GROUPED_TEST_GUILESS_MAIN(CredentialManagerKeychainTest)
#include "CredentialManagerKeychainTest.moc"
