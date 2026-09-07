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

// The reconciler's whole reason to exist is what happens when store operations are still in flight
// as new requests arrive - which the functional suite cannot reach, because its file-backed
// credential store completes every operation inline. So the reconciler is tested here against a fake
// performer that holds every completion until the test releases it, giving the test full control
// over interleaving. No profile, no CredentialManager.

#include <QtTest/QtTest>

#include <QHash>
#include <QPointer>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>
#include <vector>

#include "SignInStoreReconciler.h"
#include "utils.h"

#include "GroupedTest.h"

using Shape = SignInStoreReconciler::Shape;
using Operation = SignInStoreReconciler::Operation;
using Outcome = SignInStoreReconciler::Outcome;
using Intent = SignInStoreReconciler::Intent;

// Records every operation the reconciler asks for and keeps its Done callable until the test
// releases it, so the test decides exactly when each step "finishes".
struct FakeStore
{
    struct Call
    {
        Operation op;
        QString payload;
        SignInStoreReconciler::Done done;
    };
    std::vector<Call> calls;

    SignInStoreReconciler::Performer performer()
    {
        return [this](Operation op, QString payload, SignInStoreReconciler::Done done) {
            calls.push_back({op, std::move(payload), std::move(done)});
        };
    }

    // Complete call i. Takes the Done out first so a re-entrant call cannot see a stale one.
    void release(std::size_t i, bool ok = true, const QString& error = QString())
    {
        auto done = std::move(calls.at(i).done);
        done(ok, error);
    }

    std::vector<Operation> operations() const
    {
        std::vector<Operation> ops;
        for (const auto& call : calls) {
            ops.push_back(call.op);
        }
        return ops;
    }
};

// One entry per completion invocation, keyed by request id, so "exactly once" is checkable.
struct Outcomes
{
    struct Record
    {
        Outcome outcome;
        Operation failedAt;
        QString error;
    };
    QHash<unsigned int, std::vector<Record>> byId;

    SignInStoreReconciler::Completion recorder(unsigned int* idOut)
    {
        // The id is only known after setIntent returns, so the completion looks it up through a
        // pointer the test fills in immediately afterwards.
        return [this, idOut](Outcome outcome, Operation failedAt, QString error) {
            byId[*idOut].push_back({outcome, failedAt, std::move(error)});
        };
    }
};

static Intent fullIntent(const QString& token, const QString& provider = QString())
{
    Intent intent;
    intent.shape = Shape::Full;
    intent.account = qsl("acct:char");
    intent.provider = provider;
    intent.secureOnly = true;
    intent.token = token;
    return intent;
}

static Intent hintIntent()
{
    Intent intent;
    intent.shape = Shape::Hint;
    intent.account = qsl("acct:char");
    intent.provider = qsl("discord");
    return intent;
}

static Intent absentIntent()
{
    return Intent{};
}

class SignInStoreReconcilerTest : public QObject
{
    Q_OBJECT

private slots:
    // ---- Each shape issues exactly its sequence -----------------------------

    void fullWritesMetadataThenToken()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int id = 0;
        id = reconciler.setIntent(fullIntent(qsl("tok-1"), qsl("discord")), outcomes.recorder(&id));

        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata}));
        QVERIFY(reconciler.inFlight());
        // The metadata never carries the token, and carries secure_only for a Full intent.
        const QJsonObject metadata = QJsonDocument::fromJson(store.calls[0].payload.toUtf8()).object();
        QCOMPARE(metadata.value(qsl("account")).toString(), qsl("acct:char"));
        QCOMPARE(metadata.value(qsl("provider")).toString(), qsl("discord"));
        QCOMPARE(metadata.value(qsl("secure_only")), QJsonValue(true));
        QVERIFY(!metadata.contains(qsl("token")));

        store.release(0);
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::WriteToken}));
        QCOMPARE(store.calls[1].payload, qsl("tok-1"));

        store.release(1);
        QVERIFY(!reconciler.inFlight());
        QCOMPARE(outcomes.byId[id].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[id][0].outcome, Outcome::Reached);
    }

    void hintRemovesTokenThenWritesMetadata()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int id = 0;
        id = reconciler.setIntent(hintIntent(), outcomes.recorder(&id));

        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken}));
        store.release(0);
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken, Operation::WriteMetadata}));
        // A hint's metadata is {account, provider} only - no secure_only, exactly as before.
        const QJsonObject metadata = QJsonDocument::fromJson(store.calls[1].payload.toUtf8()).object();
        QCOMPARE(metadata.value(qsl("account")).toString(), qsl("acct:char"));
        QCOMPARE(metadata.value(qsl("provider")).toString(), qsl("discord"));
        QVERIFY(!metadata.contains(qsl("secure_only")));
        QVERIFY(!metadata.contains(qsl("token")));

        store.release(1);
        QCOMPARE(outcomes.byId[id].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[id][0].outcome, Outcome::Reached);
    }

    void absentRemovesTokenThenMetadata()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int id = 0;
        id = reconciler.setIntent(absentIntent(), outcomes.recorder(&id));

        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken}));
        store.release(0);
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken, Operation::RemoveMetadata}));
        store.release(1);
        QCOMPARE(outcomes.byId[id].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[id][0].outcome, Outcome::Reached);
    }

    // ---- Failure stops the sequence -----------------------------------------

    void metadataFailureNeverWritesTheToken()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int id = 0;
        id = reconciler.setIntent(fullIntent(qsl("tok-1")), outcomes.recorder(&id));

        store.release(0, false, qsl("disk full"));
        // The token write must never be issued: that is what keeps a fresh token from being paired
        // with metadata that never landed.
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata}));
        QVERIFY(!reconciler.inFlight());
        QCOMPARE(outcomes.byId[id].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[id][0].outcome, Outcome::Failed);
        QCOMPARE(outcomes.byId[id][0].failedAt, Operation::WriteMetadata);
        QCOMPARE(outcomes.byId[id][0].error, qsl("disk full"));
    }

    void tokenRemovalFailureNeverTouchesTheMetadata()
    {
        // The security ordering for a forget: if the token cannot be removed, the metadata must be
        // left alone, or the preferences UI loses its only way to offer the removal again.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int id = 0;
        id = reconciler.setIntent(absentIntent(), outcomes.recorder(&id));

        store.release(0, false, qsl("keychain locked"));
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken}));
        QCOMPARE(outcomes.byId[id][0].outcome, Outcome::Failed);
        QCOMPARE(outcomes.byId[id][0].failedAt, Operation::RemoveToken);
    }

    // ---- Supersession ---------------------------------------------------------

    void forgetWinsOverASaveInFlight()
    {
        // A Full is one step in; a forget arrives. The token write must never be issued - a live
        // token surviving a forget is the outcome the reconciler exists to prevent.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int saveId = 0;
        unsigned int forgetId = 0;
        saveId = reconciler.setIntent(fullIntent(qsl("tok-1")), outcomes.recorder(&saveId));
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata}));

        forgetId = reconciler.setIntent(absentIntent(), outcomes.recorder(&forgetId));
        // Nothing new is issued while the save's metadata write is still running.
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata}));

        store.release(0);
        // The save is abandoned before WriteToken; the forget starts.
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::RemoveToken}));
        QCOMPARE(outcomes.byId[saveId].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[saveId][0].outcome, Outcome::Superseded);

        store.release(1);
        store.release(2);
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::RemoveToken, Operation::RemoveMetadata}));
        QCOMPARE(outcomes.byId[forgetId].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[forgetId][0].outcome, Outcome::Reached);
    }

    void aLaterTokenWinsOverAnEarlierForget()
    {
        // Pressing "forget" once must not disable remember-me: a token minted afterwards is a new
        // request and runs in full.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int forgetId = 0;
        unsigned int saveId = 0;
        forgetId = reconciler.setIntent(absentIntent(), outcomes.recorder(&forgetId));
        store.release(0);
        store.release(1);
        QCOMPARE(outcomes.byId[forgetId][0].outcome, Outcome::Reached);

        saveId = reconciler.setIntent(fullIntent(qsl("tok-2")), outcomes.recorder(&saveId));
        store.release(2);
        store.release(3);
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken, Operation::RemoveMetadata, Operation::WriteMetadata, Operation::WriteToken}));
        QCOMPARE(store.calls[3].payload, qsl("tok-2"));
        QCOMPARE(outcomes.byId[saveId][0].outcome, Outcome::Reached);
    }

    void aRotationAbandonsTheOlderRotation()
    {
        // Two Fulls: only the newer token may reach the store, and the older request must not report
        // anything the caller would act on.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int firstId = 0;
        unsigned int secondId = 0;
        firstId = reconciler.setIntent(fullIntent(qsl("tok-A")), outcomes.recorder(&firstId));
        secondId = reconciler.setIntent(fullIntent(qsl("tok-B")), outcomes.recorder(&secondId));

        store.release(0); // first's WriteMetadata completes -> first abandoned, second starts
        store.release(1); // second's WriteMetadata
        store.release(2); // second's WriteToken
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::WriteMetadata, Operation::WriteToken}));
        QCOMPARE(store.calls[2].payload, qsl("tok-B"));
        QCOMPARE(outcomes.byId[firstId].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[firstId][0].outcome, Outcome::Superseded);
        QCOMPARE(outcomes.byId[secondId].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[secondId][0].outcome, Outcome::Reached);
    }

    void aPendingIntentReplacedBeforeStartingIsSuperseded()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int activeId = 0;
        unsigned int replacedId = 0;
        unsigned int winnerId = 0;
        activeId = reconciler.setIntent(fullIntent(qsl("tok-A")), outcomes.recorder(&activeId));
        replacedId = reconciler.setIntent(fullIntent(qsl("tok-B")), outcomes.recorder(&replacedId));
        winnerId = reconciler.setIntent(fullIntent(qsl("tok-C")), outcomes.recorder(&winnerId));

        // The replaced intent never started and is reported Superseded straight away.
        QCOMPARE(outcomes.byId[replacedId].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[replacedId][0].outcome, Outcome::Superseded);

        store.release(0);
        store.release(1);
        store.release(2);
        // tok-B never reaches the store; tok-C does.
        for (const auto& call : store.calls) {
            QVERIFY2(call.payload != qsl("tok-B"), "a replaced intent's token must never be written");
        }
        QCOMPARE(store.calls.back().payload, qsl("tok-C"));
        QCOMPARE(outcomes.byId[activeId][0].outcome, Outcome::Superseded);
        QCOMPARE(outcomes.byId[winnerId][0].outcome, Outcome::Reached);
    }

    void everyCompletionIsInvokedExactlyOnceAcrossABurst()
    {
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        std::vector<std::unique_ptr<unsigned int>> ids;
        const std::vector<Intent> burst{fullIntent(qsl("t1")), hintIntent(), absentIntent(), fullIntent(qsl("t4")), hintIntent()};
        for (const auto& intent : burst) {
            ids.push_back(std::make_unique<unsigned int>(0));
            *ids.back() = reconciler.setIntent(intent, outcomes.recorder(ids.back().get()));
        }
        // Drain whatever the reconciler issues until it is idle.
        std::size_t next = 0;
        while (reconciler.inFlight()) {
            QVERIFY2(next < store.calls.size(), "the reconciler is in flight but issued nothing to complete");
            store.release(next++);
        }
        for (const auto& id : ids) {
            QCOMPARE(outcomes.byId[*id].size(), std::size_t{1});
        }
        // The last request is the one that reached the store; every earlier one was superseded.
        QCOMPARE(outcomes.byId[*ids.back()][0].outcome, Outcome::Reached);
        for (std::size_t i = 0; i + 1 < ids.size(); ++i) {
            QCOMPARE(outcomes.byId[*ids[i]][0].outcome, Outcome::Superseded);
        }
    }

    // ---- Lifetime -------------------------------------------------------------

    void aDoneArrivingAfterDestructionIsDiscarded()
    {
        FakeStore store;
        Outcomes outcomes;
        auto reconciler = std::make_unique<SignInStoreReconciler>(store.performer());
        unsigned int id = 0;
        id = reconciler->setIntent(fullIntent(qsl("tok-1")), outcomes.recorder(&id));
        QCOMPARE(store.calls.size(), std::size_t{1});

        reconciler.reset();
        // The held Done must be safe to invoke now; the reconciler is gone and nothing may happen.
        store.release(0);
        QVERIFY(outcomes.byId[id].empty());
        QCOMPARE(store.calls.size(), std::size_t{1});
    }

    // ---- The token only ever reaches the store by being written ---------------

    void aTokenDroppedBeforeItsWriteNeverReachesTheStore()
    {
        // Three ways a Full intent can be dropped before WriteToken: replaced while pending,
        // abandoned while active, and failed at the metadata step. In none of them may the token
        // appear in any payload the store was handed. (The internal zeroing that accompanies each of
        // these is not observable from outside; what is observable, and what matters, is that the
        // secret never left this object.)
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int a = 0;
        unsigned int b = 0;
        unsigned int c = 0;
        unsigned int d = 0;
        a = reconciler.setIntent(fullIntent(qsl("dropped-by-failure")), outcomes.recorder(&a));
        store.release(0, false, qsl("nope"));

        b = reconciler.setIntent(fullIntent(qsl("dropped-by-abandon")), outcomes.recorder(&b));
        c = reconciler.setIntent(fullIntent(qsl("dropped-by-replace")), outcomes.recorder(&c));
        d = reconciler.setIntent(fullIntent(qsl("the-only-token-that-lands")), outcomes.recorder(&d));
        std::size_t next = 1;
        while (reconciler.inFlight()) {
            store.release(next++);
        }

        QStringList written;
        for (const auto& call : store.calls) {
            if (call.op == Operation::WriteToken) {
                written << call.payload;
            }
        }
        QCOMPARE(written, QStringList{qsl("the-only-token-that-lands")});
    }
};

#include "SignInStoreReconcilerTest.moc"
MUDLET_GROUPED_TEST_MAIN(SignInStoreReconcilerTest)
