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

// A performer that completes every operation immediately, inline - the production behaviour on
// the file-backed credential store, and the reason the functional suite cannot reach the races
// FakeStore exists to test. Used to confirm the reconciler behaves the same way when there is
// never anything actually in flight.
struct SyncStore
{
    struct Call
    {
        Operation op;
        QString payload;
    };
    std::vector<Call> calls;

    SignInStoreReconciler::Performer performer()
    {
        return [this](Operation op, QString payload, SignInStoreReconciler::Done done) {
            calls.push_back({op, payload});
            done(true, QString());
        };
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

    // ---- Re-entrancy: a Completion calling setIntent() again -------------------

    void reenteringSetIntentFromASupersededCompletionPreservesTheReplacement()
    {
        // Regression: displacing a still-pending request used to run its Superseded completion
        // before the new request was recorded as pending. If that completion itself calls
        // setIntent() - easy for Task 2's caller to do from a "saved sign-in changed" handler -
        // the request it sets was silently overwritten a moment later, and its completion was
        // never invoked.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int idA = 0;
        unsigned int idB = 0;
        unsigned int idD = 0;
        int completionsC = 0;
        Outcome resultC = Outcome::Failed;

        idA = reconciler.setIntent(fullIntent(qsl("tok-A")), outcomes.recorder(&idA));
        idB = reconciler.setIntent(fullIntent(qsl("tok-B")), [&](Outcome outcome, Operation failedAt, QString error) {
            outcomes.byId[idB].push_back({outcome, failedAt, std::move(error)});
            if (outcome == Outcome::Superseded) {
                // Fires while C's setIntent() call below is still on the stack.
                idD = reconciler.setIntent(fullIntent(qsl("tok-D")), outcomes.recorder(&idD));
            }
        });
        // Not Outcomes::recorder() for C: with the bug fixed, C's own Superseded completion fires
        // synchronously nested inside this very call - B's completion above sets D, which displaces
        // C - before the assignment below would have happened, so a pointer-to-id lookup would see
        // the id's old value. See the comment on the SyncStore-based tests further down for the
        // same hazard in a simpler shape.
        reconciler.setIntent(fullIntent(qsl("tok-C")), [&](Outcome outcome, Operation, QString) {
            ++completionsC;
            resultC = outcome;
        });

        std::size_t next = 0;
        while (reconciler.inFlight()) {
            QVERIFY2(next < store.calls.size(), "the reconciler is in flight but issued nothing to complete");
            store.release(next++);
        }

        // Every one of the four requests must be accounted for exactly once - D above all, since
        // it is the one the bug used to drop on the floor.
        QCOMPARE(outcomes.byId[idA].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idB].size(), std::size_t{1});
        QCOMPARE(completionsC, 1);
        QCOMPARE(resultC, Outcome::Superseded);
        QCOMPARE(outcomes.byId[idD].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idB][0].outcome, Outcome::Superseded);
        QCOMPARE(outcomes.byId[idD][0].outcome, Outcome::Reached);

        QStringList written;
        for (const auto& call : store.calls) {
            if (call.op == Operation::WriteToken) {
                written << call.payload;
            }
        }
        QCOMPARE(written, QStringList{qsl("tok-D")});
    }

    void reenteringSetIntentFromAnAbandonedActiveCompletionPreservesTheReplacement()
    {
        // The third re-entrancy path named for finding 2: an ACTIVE request abandoned mid-sequence
        // (as opposed to a still-pending one, covered above), whose Superseded completion itself
        // calls setIntent() again. onStepDone()'s abandon branch starts the newer, already-pending
        // request before finishing the abandoned one specifically so mActive is never empty at that
        // point - swap that order and a re-entrant setIntent() here finds mPending already consumed
        // by a nested start(), then the outer function's own start() dereferences an empty mPending.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int idA = 0;
        unsigned int idB = 0;
        unsigned int idC = 0;

        idA = reconciler.setIntent(fullIntent(qsl("tok-A")), [&](Outcome outcome, Operation failedAt, QString error) {
            outcomes.byId[idA].push_back({outcome, failedAt, std::move(error)});
            if (outcome == Outcome::Superseded) {
                // Fires from inside the abandon branch below, while B (already pending) is starting.
                idC = reconciler.setIntent(fullIntent(qsl("tok-C")), outcomes.recorder(&idC));
            }
        });
        idB = reconciler.setIntent(fullIntent(qsl("tok-B")), outcomes.recorder(&idB));
        // A is active and mid-sequence - its WriteMetadata is in flight - when B arrives and queues.
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata}));
        QVERIFY(reconciler.inFlight());

        std::size_t next = 0;
        while (reconciler.inFlight()) {
            QVERIFY2(next < store.calls.size(), "the reconciler is in flight but issued nothing to complete");
            store.release(next++);
        }

        // Every one of the three requests must be accounted for exactly once - C above all, since a
        // start()/finish() reorder in the abandon branch is what would drop it.
        QCOMPARE(outcomes.byId[idA].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idA][0].outcome, Outcome::Superseded);
        QCOMPARE(outcomes.byId[idB].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idB][0].outcome, Outcome::Superseded);
        QCOMPARE(outcomes.byId[idC].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idC][0].outcome, Outcome::Reached);

        QStringList written;
        for (const auto& call : store.calls) {
            if (call.op == Operation::WriteToken) {
                written << call.payload;
            }
        }
        QCOMPARE(written, QStringList{qsl("tok-C")});
    }

    void reachedCompletionCanReenterSetIntentAndTheNewRequestRuns()
    {
        // Pins the restart in runStep() after a Reached completion sets a new intent: the
        // reconciler must actually start it, and must not also try to start it a second time
        // once runStep() itself notices mPending.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int idA = 0;
        unsigned int idB = 0;
        idA = reconciler.setIntent(hintIntent(), [&](Outcome outcome, Operation failedAt, QString error) {
            outcomes.byId[idA].push_back({outcome, failedAt, std::move(error)});
            if (outcome == Outcome::Reached) {
                idB = reconciler.setIntent(fullIntent(qsl("tok-B")), outcomes.recorder(&idB));
            }
        });

        store.release(0); // RemoveToken
        store.release(1); // WriteMetadata -> A reaches; its completion sets B, reentrant, right here
        QCOMPARE(outcomes.byId[idA].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idA][0].outcome, Outcome::Reached);
        QVERIFY(reconciler.inFlight());
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::RemoveToken, Operation::WriteMetadata, Operation::WriteMetadata}));

        store.release(2);
        store.release(3);
        QCOMPARE(outcomes.byId[idB].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idB][0].outcome, Outcome::Reached);
        QCOMPARE(store.calls.back().payload, qsl("tok-B"));
    }

    void failedCompletionCanReenterSetIntentAndTheFallbackRuns()
    {
        // Pins the restart in onStepDone() after a Failed completion sets a new intent - Task 2's
        // storeResumeHint fallback depends on being able to do exactly this (fall back to a
        // forget) directly from inside a Failed completion, and have it actually run rather than
        // being silently dropped.
        FakeStore store;
        Outcomes outcomes;
        SignInStoreReconciler reconciler(store.performer());
        unsigned int idA = 0;
        unsigned int idB = 0;
        idA = reconciler.setIntent(fullIntent(qsl("tok-A")), [&](Outcome outcome, Operation failedAt, QString error) {
            outcomes.byId[idA].push_back({outcome, failedAt, std::move(error)});
            if (outcome == Outcome::Failed) {
                idB = reconciler.setIntent(absentIntent(), outcomes.recorder(&idB));
            }
        });

        store.release(0, false, qsl("disk full")); // A's WriteMetadata fails -> fallback sets B, reentrant
        QCOMPARE(outcomes.byId[idA].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idA][0].outcome, Outcome::Failed);
        QVERIFY(reconciler.inFlight());
        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::RemoveToken}));

        store.release(1);
        store.release(2);
        QCOMPARE(outcomes.byId[idB].size(), std::size_t{1});
        QCOMPARE(outcomes.byId[idB][0].outcome, Outcome::Reached);
    }

    // ---- A fully synchronous Performer - the production, file-backed path ------

    void aSynchronousPerformerCompletesAFullSequenceInline()
    {
        // The file-backed credential store completes every operation inline, which the
        // FakeStore-based tests above cannot exercise. setIntent() must return only once the
        // whole synchronous sequence - here, a Full - has already run to completion.
        //
        // Outcomes::recorder() is deliberately not used here: it looks a request's id up through a
        // pointer the test fills in *after* setIntent() returns, which assumes the completion fires
        // later still. A synchronous performer fires it before setIntent() returns, while that
        // pointer's target is still its old value - so the completion is captured directly instead.
        SyncStore store;
        SignInStoreReconciler reconciler(store.performer());
        int completions = 0;
        Outcome result = Outcome::Failed;
        reconciler.setIntent(fullIntent(qsl("tok-1")), [&](Outcome outcome, Operation, QString) {
            ++completions;
            result = outcome;
        });

        QCOMPARE(store.operations(), (std::vector<Operation>{Operation::WriteMetadata, Operation::WriteToken}));
        QVERIFY(!reconciler.inFlight());
        QCOMPARE(completions, 1);
        QCOMPARE(result, Outcome::Reached);
    }

    void aSynchronousPerformerAbandonsARequestSupersededMidSequence()
    {
        // With a synchronous store, the only way a second request can arrive while the first is
        // still running is reentrancy from within the performer itself - simulated here to prove
        // the abandon path in onStepDone() still works, and nothing crashes, when there is never
        // a moment where the reconciler is waiting on anything. Completions are captured directly,
        // not through Outcomes::recorder() - see the comment in the test above this one.
        SignInStoreReconciler* reconciler = nullptr;
        std::vector<Operation> ops;
        bool triggered = false;
        int completionsA = 0;
        int completionsB = 0;
        Outcome resultA = Outcome::Failed;
        Outcome resultB = Outcome::Failed;

        auto performer = [&](Operation op, QString payload, SignInStoreReconciler::Done done) {
            ops.push_back(op);
            if (!triggered && op == Operation::WriteMetadata) {
                triggered = true;
                reconciler->setIntent(absentIntent(), [&](Outcome outcome, Operation, QString) {
                    ++completionsB;
                    resultB = outcome;
                });
            }
            done(true, QString());
        };
        SignInStoreReconciler r(performer);
        reconciler = &r;

        r.setIntent(fullIntent(qsl("tok-A")), [&](Outcome outcome, Operation, QString) {
            ++completionsA;
            resultA = outcome;
        });

        QCOMPARE(ops, (std::vector<Operation>{Operation::WriteMetadata, Operation::RemoveToken, Operation::RemoveMetadata}));
        QCOMPARE(completionsA, 1);
        QCOMPARE(resultA, Outcome::Superseded);
        QCOMPARE(completionsB, 1);
        QCOMPARE(resultB, Outcome::Reached);
    }
};

#include "SignInStoreReconcilerTest.moc"
MUDLET_GROUPED_TEST_MAIN(SignInStoreReconcilerTest)
