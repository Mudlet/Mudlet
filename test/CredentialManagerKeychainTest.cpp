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
// The migration scenarios only exist on Windows, so those tests skip elsewhere (the bodies
// still compile on all platforms). testFallbackChainRunsUnderATimeout is the exception: the
// fallback chain it guards runs on every platform, so it does not skip.

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
    void testFallbackChainRunsUnderATimeout();
    void testANestedOperationDoesNotSwallowTheCascadeResult();

private:
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
// late finish after a timeout cannot write to a dead stack frame
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
    job->disconnect();
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
    job->disconnect();
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
    job->disconnect();
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

// Records children appearing on a watched object. QChildEvent is delivered synchronously from the
// QObject constructor, so this notices a guard being armed even on a backend that answers the whole
// cascade in the same event-loop pass - which polling for an *active* timer cannot. The child is only
// a QObject at that point (its QTimer constructor has not run), so the type is decided later, on
// demand, rather than in the filter.
class ChildArrivalWatcher : public QObject
{
public:
    bool sawTimer() const
    {
        for (const auto& child : mChildren) {
            if (qobject_cast<QTimer*>(child)) {
                return true;
            }
        }
        return false;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::ChildAdded) {
            mChildren.append(QPointer<QObject>(static_cast<QChildEvent*>(event)->child()));
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QList<QPointer<QObject>> mChildren;
};

} // namespace

void CredentialManagerKeychainTest::initTestCase()
{
    // CredentialManagerTest forces file storage via this variable; this test exists to
    // exercise the real credential store, so make sure it is not inherited from the
    // environment
    qunsetenv("MUDLET_TEST_MODE");

    if (!onWindows()) {
        return;
    }

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
    if (!onWindows() || !mStoreAvailable) {
        return;
    }

    // Remove every TargetName a test could have created or migrated to, plus the
    // encrypted-file fallback copy
    const QString service = expectedServiceName(mProfile, mKey);
    const QString legacyService = expectedLegacyServiceName(mProfile, mKey);
    const QStringList targets = {service, combinedTargetName(service), legacyService, combinedTargetName(legacyService), mKey};

    for (const QString& target : targets) {
        deleteTarget(target);
    }

    CredentialManager::removeCredential(mProfile, mKey);
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

void CredentialManagerKeychainTest::testFallbackChainRunsUnderATimeout()
{
    // Not Windows-gated: every platform runs the fallback chain, and this is about the chain being
    // guarded at all rather than about any historical entry layout.
    //
    // retrieveCredential arms a timeout for its first read and cancels it the moment that read
    // answers. A read answering "no entry in the new format" - the ordinary case for any profile that
    // has never saved a password - then hands off to a chain several more keychain jobs deep. Those
    // jobs used to run with nothing armed, so one that never answered left the caller waiting for
    // good: dlgConnectionProfiles latches mKeychainOperationInProgress on such a read and clears it
    // only from the callback, so the profiles dialog could never be accepted again.
    //
    // The stall is not hypothetical on a developer machine - the old-format probe below routinely
    // fails to answer on macOS - which is why this asserts the guard rather than waiting one out.
    QPointer<CredentialManager> manager = new CredentialManager;

    bool answered = false;
    bool reportedSuccess = true;
    QString reportedError;
    manager->retrievePassword(mProfile, mKey, [&](bool success, QString, const QString& errorMessage) {
        answered = true;
        reportedSuccess = success;
        reportedError = errorMessage;
    });

    auto* readJob = manager->findChild<QKeychain::ReadPasswordJob*>();
    QVERIFY2(readJob, "retrievePassword should have started a keychain read");

    // Watch for the guard being armed rather than trying to catch it between event-loop passes. A
    // backend that answers the whole cascade promptly - a CI box with no secret service, say - would
    // otherwise leave nothing active to observe, and the test would fail for being fast rather than
    // for being unguarded.
    ChildArrivalWatcher armingWatcher;
    manager->installEventFilter(&armingWatcher);

    // Drive the hand-off deterministically rather than depending on what this machine's store says
    // about an entry that does not exist.
    readJob->emitFinishedWithError(QKeychain::EntryNotFound, QStringLiteral("synthetic: no such entry"));
    // The completion is a queued connection, so one pass delivers it and starts the cascade.
    QCoreApplication::processEvents();

    // Collapse each guard as it appears, standing in for a stage whose keychain job never answers.
    // Waiting the real interval out would cost half a minute per stage.
    //
    // retrievePassword is a cascade: each stage's failure starts the next, and only the last -
    // encrypted file storage, which is synchronous - answers the caller. So a timeout does not report
    // failure directly, it advances the cascade. The property that matters is that the cascade still
    // reaches its end when every keychain stage stalls, instead of stopping on one that never answers.
    for (int stage = 0; stage < 20 && !answered; ++stage) {
        QTimer* guard = nullptr;
        const auto timers = manager ? manager->findChildren<QTimer*>() : QList<QTimer*>{};
        for (auto* candidate : timers) {
            // The armed one specifically: setupTimeout() replaces the timer by disconnecting the old
            // one and deleteLater()ing it, so for one event-loop pass both are children.
            if (candidate->isActive()) {
                guard = candidate;
                break;
            }
        }
        if (!guard) {
            // No stage in flight and nobody has answered: either the cascade is between stages, or it
            // is stuck on an unguarded one. qWaitFor decides which.
            QTest::qWait(20);
            continue;
        }
        guard->setInterval(0);
        guard->start();
        QTest::qWait(20);
    }

    // A cascade that has already answered never needed a guard, and on a backend with no secret
    // service the whole thing can resolve in one pass. Requiring the guard unconditionally would fail
    // such a run for being fast rather than for being unguarded, so the assertion is that the cascade
    // did one or the other - and on any machine where a stage actually stalls, only the guard can
    // satisfy it.
    QVERIFY2(armingWatcher.sawTimer() || answered, "the fallback cascade neither armed a timeout nor answered, so a keychain job that never answers would strand the caller");
    QVERIFY2(QTest::qWaitFor(
                     [&answered]() {
                         return answered;
                     },
                     5000),
             "a fallback cascade whose keychain stages all stall must still report an outcome to its caller");
    QVERIFY2(!reportedSuccess, "a retrieval that found nothing must be reported as a failure, not as an empty success");

    delete manager;
}

void CredentialManagerKeychainTest::testANestedOperationDoesNotSwallowTheCascadeResult()
{
    // A cascade that recovers a password from the legacy or colliding format calls storePassword()
    // part-way through itself, and that runs cleanupCurrentOperation() and setupTimeout() on the same
    // manager - wiping the shared retrieval callback and replacing the shared timer while the cascade
    // is still running. A cascade anchored to either loses its own result: the password is recovered
    // and then dropped, and the caller waits for good, which is the very hang this class is meant to
    // have stopped.
    //
    // A second retrieval stands in for that nested operation. It clobbers exactly the same shared
    // state, and unlike a store it writes nothing to the keychain of whoever runs the suite.
    QPointer<CredentialManager> manager = new CredentialManager;

    bool answered = false;
    manager->retrievePassword(mProfile, mKey, [&answered](bool, QString, const QString&) {
        answered = true;
    });

    auto* readJob = manager->findChild<QKeychain::ReadPasswordJob*>();
    QVERIFY2(readJob, "retrievePassword should have started a keychain read");
    readJob->emitFinishedWithError(QKeychain::EntryNotFound, QStringLiteral("synthetic: no such entry"));
    QCoreApplication::processEvents();

    // Mid-cascade, exactly where a migration would store what it recovered.
    manager->retrievePassword(mProfile + QStringLiteral("-nested"), mKey, [](bool, QString, const QString&) {});
    QCoreApplication::processEvents();

    // Collapse guards as they appear so no stage has to be waited out.
    for (int stage = 0; stage < 40 && !answered; ++stage) {
        QTimer* guard = nullptr;
        const auto timers = manager ? manager->findChildren<QTimer*>() : QList<QTimer*>{};
        for (auto* candidate : timers) {
            if (candidate->isActive()) {
                guard = candidate;
                break;
            }
        }
        if (!guard) {
            QTest::qWait(20);
            continue;
        }
        guard->setInterval(0);
        guard->start();
        QTest::qWait(20);
    }

    QVERIFY2(QTest::qWaitFor(
                     [&answered]() {
                         return answered;
                     },
                     5000),
             "a cascade must still answer its caller after another operation reset the manager's shared state");

    delete manager;
}

QTEST_GUILESS_MAIN(CredentialManagerKeychainTest)
#include "CredentialManagerKeychainTest.moc"
