#ifndef MUDLET_SIGNINSTORERECONCILER_H
#define MUDLET_SIGNINSTORERECONCILER_H

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

#include <QObject>
#include <QString>

#include <functional>
#include <optional>
#include <vector>

// Drives a profile's stored sign-in - two credential entries, the metadata and the token - toward
// the most recently requested end state, one store operation at a time.
//
// It exists because the store operations are asynchronous and used to be issued independently from
// three places, so they could interleave: a rotation could land an already-spent token last, and a
// "forget" could be outrun by a save it was meant to cancel. Here exactly one sequence runs at a
// time, a newer request abandons the running one before its next step, and only the newest request
// reports an outcome the caller should act on.
//
// The class knows nothing about where the entries live: the caller supplies a Performer that does
// one operation and calls back. That is what lets a test drive it with complete control over when
// each operation completes.
//
// Serialising every mutation has a cost: a stalled store operation now delays every later sign-in
// mutation behind it, up to CredentialManager's own timeout per step - about twice that for a
// two-step sequence. Concretely, "Forget saved sign-in" can take up to a minute to answer where it
// previously raced ahead of a slow save. Bounded and correct, but worth knowing before you go
// looking for why a UI action seems to hang.
class SignInStoreReconciler : public QObject
{
    Q_OBJECT

public:
    // The three valid end states of the stored sign-in.
    enum class Shape {
        Absent, // nothing stored
        Hint,   // {account, provider} and no token: enough to resume a browser sign-in later
        Full    // {account, provider?, secure_only} plus the token
    };

    struct Intent
    {
        Shape shape = Shape::Absent;
        QString account;
        QString provider;
        bool secureOnly = true;
        // Full only. A bearer secret: moved into the store's write when that step runs, and scrubbed
        // on every path where the intent is dropped before then.
        QString token;
    };

    enum class Operation { WriteMetadata, WriteToken, RemoveToken, RemoveMetadata };

    enum class Outcome {
        Reached,   // the store now holds the requested end state
        Failed,    // an operation failed; failedAt names it and nothing further was attempted
        Superseded // a newer intent replaced this one before it completed; nothing to report
    };

    using Done = std::function<void(bool ok, QString error)>;
    // Perform one operation. payload is the metadata JSON for WriteMetadata, the token for
    // WriteToken, and empty for the removals. Must call done exactly once, on any thread affinity the
    // reconciler's own; a done arriving after the reconciler is destroyed is discarded. A Performer
    // that never calls done at all leaves the reconciler permanently occupied - see setIntent().
    using Performer = std::function<void(Operation op, QString payload, Done done)>;
    // failedAt is meaningful only when outcome is Failed.
    using Completion = std::function<void(Outcome outcome, Operation failedAt, QString error)>;

    explicit SignInStoreReconciler(Performer performer, QObject* parent = nullptr);
    ~SignInStoreReconciler() override;

    // Requests an end state. Replaces any request that has not started yet (which completes as
    // Superseded) and, if a request is running, abandons it before its next operation. Every
    // request's completion is invoked exactly once - Reached, Failed or Superseded - with two
    // exceptions: the reconciler is destroyed first, in which case none are, since their owner is
    // going with it; or the active request's Performer never calls its Done at all, which leaves
    // mActive occupied forever and every later request either superseded or never run. The latter is
    // not hypothetical: CredentialManager drops its callback outright at application shutdown (its
    // isOperationValid() returns false and the callback is never invoked), tracked in #10587. Returns
    // the id assigned, in case the caller wants to correlate.
    // Pass intent as an rvalue (std::move it in) when it carries a Full token: a dropped request's
    // token is scrubbed before its completion runs, but that scrub only reaches the real bytes
    // when this call is the token's sole owner - a copy elsewhere in the caller survives it intact.
    unsigned int setIntent(Intent intent, Completion completion);
    bool inFlight() const { return mActive.has_value(); }

private:
    struct Request
    {
        unsigned int id = 0;
        Intent intent;
        Completion completion;
        std::size_t step = 0;
    };

    static std::vector<Operation> sequenceFor(Shape shape);
    static QString metadataPayload(const Intent& intent);
    static void scrub(Request& request);
    void start();
    void runStep();
    void onStepDone(unsigned int id, Operation op, bool ok, QString error);
    void finish(Request& request, Outcome outcome, Operation failedAt, QString error);

    Performer mPerformer;
    unsigned int mNextId = 0;
    std::optional<Request> mActive;
    std::optional<Request> mPending;
};

#endif // MUDLET_SIGNINSTORERECONCILER_H
