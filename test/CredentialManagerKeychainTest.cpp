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

#include <CredentialManager.h>
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
// they care about; the two that plant entries under their real service skip when no credential store
// is available.

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
    void testALookupAnswersWhicheverLaterReadStalls_data();
    void testALookupAnswersWhicheverLaterReadStalls();
    void testALookupReadsEachPlaceOnceInOrder_data();
    void testALookupReadsEachPlaceOnceInOrder();
    void testATimedOutRemovalDoesNotStopLaterKeychainJobs();
    void testALookupIsNotDisturbedByAnotherOnTheSameManager();
    void testDeletingAManagerMidLookupLeavesItsReadToFinish();
    void testACallbackThatFlushesDeferredDeletesDoesNotDeleteTheAnsweringRead();
    void testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat_data();
    void testALookupFindsThePasswordInTheEncryptedFileBeforeTheCollidingFormat();
    void testARecoveredPasswordIsReturnedWhileItsMigrationStalls();
    void testARecoveredPasswordSurvivesAKeychainThatRefusesToStoreIt();
    void testAKeychainErrorIsReportedRatherThanNothingFound_data();
    void testAKeychainErrorIsReportedRatherThanNothingFound();

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
QString expectedServiceName(const QString& profileName, const QString& key)
{
    static const QRegularExpression sanitizePattern(QStringLiteral(R"REGEX([^\w\-\.])REGEX"));

    const QByteArray data = QStringLiteral("%1:%2").arg(profileName, key).toUtf8();
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
    QList<ExpectedRead> reads;
    reads.append({QStringLiteral("current format"), service, service});
#if defined(Q_OS_WIN)
    reads.append({QStringLiteral("pre-0.17 naming"), QString(), service});
    reads.append({QStringLiteral("old key=account format"), QString(), key});
#else
    reads.append({QStringLiteral("old key=account format"), service, key});
#endif
    if (key == QStringLiteral("character") || key == QStringLiteral("password")) {
        reads.append({QStringLiteral("pre-4.20.0 format"), QStringLiteral("Mudlet profile"), profileName});
    }
    reads.append({QStringLiteral("colliding format"), legacyService, legacyService});
#if defined(Q_OS_WIN)
    reads.append({QStringLiteral("colliding format under pre-0.17 naming"), QString(), legacyService});
#else
    reads.append({QStringLiteral("colliding format with key=account"), legacyService, key});
#endif
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
            if (mShouldStall && mShouldStall(job)) {
                watch(job);
                mStalled.append(job);
                return false;
            }
            if (read && mAnswerReadsNotFound) {
                watch(job);
                QTimer::singleShot(0, job, [job, error = mNotFoundError]() {
                    answer(job, error, QStringLiteral("synthetic: nothing found"));
                });
                return false;
            }
            return true;
        };
    }

    const QList<QPair<QString, QString>>& reads() const { return mReads; }

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

    bool firstStalledAlive() const { return !mStalled.isEmpty() && mStalled.constFirst(); }

    // Answers a job the staller took over, with an error of the test's choosing
    static void answer(QKeychain::Job* job, QKeychain::Error error, const QString& message) { job->emitFinishedWithError(error, message); }

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
    QObject mWitness;
    int mOutsideAnswers = 0;
    int mSeen = 0;
    bool mAnswerReadsNotFound = false;
    QKeychain::Error mNotFoundError = QKeychain::EntryNotFound;
};

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

bool waitForAnswer(const std::shared_ptr<Answer>& answer, int timeoutMs = kWaitMs)
{
    return QTest::qWaitFor(
            [answer]() {
                return answer->count > 0;
            },
            timeoutMs);
}

} // namespace

Q_DECLARE_METATYPE(QKeychain::Error)

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

    // On every platform: the Windows migration tests need it, and so do the tests of the lookup chain
    // that need its reads to be answered.
    const QString probe = QStringLiteral("MudletKCTest-probe-%1").arg(QUuid::createUuid().toString(QUuid::Id128).left(8));
    mStoreAvailable = writeTarget(probe, QStringLiteral("probe")) && deleteTarget(probe);

    if (!mStoreAvailable) {
        qWarning() << "CredentialManagerKeychainTest: credential store unavailable, tests will skip";
    }
}

void CredentialManagerKeychainTest::init()
{
    const QString runId = QUuid::createUuid().toString(QUuid::Id128).left(8);
    mProfile = QStringLiteral("MudletKCTest-%1").arg(runId);
    mKey = QStringLiteral("kctest_%1").arg(runId);
}

void CredentialManagerKeychainTest::cleanup()
{
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
    QTest::qWait(100);
    QVERIFY2(staller.firstStalledAlive(), "a read still waiting on the keychain was deleted");

    // Once the keychain answers, the read deletes itself - and the answer has to reach receivers other
    // than the manager, since QtKeychain's queue moves on through its own connection to the job.
    const int released = staller.release();
    QCOMPARE(staller.answersToOtherReceivers(), released);
    QVERIFY2(keychainQueueRuns(), "QtKeychain's queue did not move on once the stalled read answered");
    QTRY_VERIFY2(!staller.firstStalledAlive(), "a read that answered after its lookup gave up was never deleted");
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
    QTest::qWait(200);
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
    QTest::qWait(200);
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
    QTest::qWait(100);
    QCOMPARE(answer->count, 0);

    staller.release();
    QVERIFY2(keychainQueueRuns(), "QtKeychain's queue did not move on once the orphaned read answered");
    QTRY_VERIFY2(!staller.firstStalledAlive(), "an orphaned read was never deleted once it answered");
    QCOMPARE(answer->count, 0);
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
    QTest::qWait(100);
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
    // looking in the others
    QCOMPARE(staller.reads().size(), expectedReads(mProfile, mKey).size());
}

QTEST_GUILESS_MAIN(CredentialManagerKeychainTest)
#include "CredentialManagerKeychainTest.moc"
