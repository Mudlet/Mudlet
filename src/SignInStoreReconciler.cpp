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

#include "SignInStoreReconciler.h"

#include "SecureStringUtils.h"
#include "utils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>

#include <utility>

SignInStoreReconciler::SignInStoreReconciler(Performer performer, QObject* parent)
: QObject(parent)
, mPerformer(std::move(performer))
{
}

SignInStoreReconciler::~SignInStoreReconciler()
{
    // Whatever never reached the store is dropped with its secret zeroed. The completions are not
    // invoked: whoever supplied them is being torn down alongside this object.
    if (mActive) {
        scrub(*mActive);
    }
    if (mPending) {
        scrub(*mPending);
    }
}

SignInStoreReconciler::Intent SignInStoreReconciler::Intent::full(QString account, QString provider, bool secureOnly, QString token)
{
    Intent intent;
    intent.shape = Shape::Full;
    intent.account = std::move(account);
    intent.provider = std::move(provider);
    intent.secureOnly = secureOnly;
    intent.token = std::move(token);
    return intent;
}

SignInStoreReconciler::Intent SignInStoreReconciler::Intent::hint(QString account, QString provider)
{
    Intent intent;
    intent.shape = Shape::Hint;
    intent.account = std::move(account);
    intent.provider = std::move(provider);
    return intent;
}

SignInStoreReconciler::Intent SignInStoreReconciler::Intent::absent()
{
    return Intent{};
}

std::vector<SignInStoreReconciler::Operation> SignInStoreReconciler::sequenceFor(Shape shape)
{
    // The write goes metadata first and the token only after, so a failure part-way leaves a resume
    // hint rather than a token paired with metadata that never landed. The removals go token first,
    // so a failure part-way leaves harmless metadata rather than an orphaned secret - and leaves the
    // metadata the preferences UI keys "Forget saved sign-in" on, so the player can retry.
    switch (shape) {
    case Shape::Full:
        return {Operation::WriteMetadata, Operation::WriteToken};
    case Shape::Hint:
        return {Operation::RemoveToken, Operation::WriteMetadata};
    case Shape::Absent:
        return {Operation::RemoveToken, Operation::RemoveMetadata};
    }
    return {};
}

QString SignInStoreReconciler::metadataPayload(const Intent& intent)
{
    // Never the token: keeping the secret out of this JSON is the point of the split storage.
    QJsonObject obj;
    obj[qsl("account")] = intent.account;
    if (!intent.provider.isEmpty()) {
        obj[qsl("provider")] = intent.provider;
    }
    if (intent.shape == Shape::Full) {
        obj[qsl("secure_only")] = intent.secureOnly;
    }
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void SignInStoreReconciler::scrub(Request& request)
{
    // secureStringClear() only reaches the real bytes when the token is uniquely owned at this
    // point. Intent is move-only, so a caller cannot copy one in by accident; it can still leave a
    // second owner behind by assigning intent.token from a QString it keeps, which no type can
    // prevent - that caller gets a private detach-and-zero leaving the original bytes untouched.
    SecureStringUtils::secureStringClear(request.intent.token);
}

unsigned int SignInStoreReconciler::setIntent(Intent intent, Completion completion)
{
    const auto id = ++mNextId;
    // Assign the new pending request before finishing whatever it displaced: that completion may
    // itself call setIntent() again, and if it did so while the old request was still sitting in
    // mPending, its own request would be the one silently overwritten a moment later instead of
    // this one.
    auto replaced = std::exchange(mPending, std::optional<Request>(Request{id, std::move(intent), std::move(completion), 0}));
    if (replaced) {
        // Replaced before it ever started: it will never reach the store, so say so now and drop
        // its secret. Invoking the completion here, rather than never, is what keeps "exactly once"
        // true for every request.
        finish(*replaced, Outcome::Superseded, Operation::WriteMetadata, QString());
    }
    if (!mActive) {
        start();
    }
    return id;
}

void SignInStoreReconciler::start()
{
    mActive = std::move(*mPending);
    mPending.reset();
    runStep();
}

void SignInStoreReconciler::runStep()
{
    const auto sequence = sequenceFor(mActive->intent.shape);
    if (mActive->step >= sequence.size()) {
        // Move the request out before reporting, so a completion that immediately sets a new intent
        // finds the reconciler idle rather than re-entering a request still marked active.
        auto reached = std::move(*mActive);
        // mActive must already be empty when finish() runs: a completion that calls setIntent()
        // relies on finding no active request here so it can self-start via setIntent()'s own
        // "if (!mActive) { start(); }" - reordering these two lines would leave that self-start
        // unreachable and the re-entrant request would never run.
        mActive.reset();
        finish(reached, Outcome::Reached, Operation::WriteMetadata, QString());
        return;
    }

    const auto op = sequence[mActive->step];
    QString payload;
    if (op == Operation::WriteMetadata) {
        payload = metadataPayload(mActive->intent);
    } else if (op == Operation::WriteToken) {
        // A genuine handover, not a copy: the performer receives the sole copy this object held, and
        // the request keeps nothing (scrub() then finds an empty string and does nothing, which is the
        // intent). Load-bearing for the performer's own clear, not for scrub() - a copy here would
        // leave the performer zeroing one of two live buffers, so the secret would outlive the write.
        payload = std::move(mActive->intent.token);
    }

    issue(op, std::move(payload));
}

void SignInStoreReconciler::issue(Operation op, QString payload)
{
    // The performer may complete synchronously or long after this object is gone; the QPointer makes
    // a late completion a no-op instead of a use-after-free.
    const auto id = mActive->id;
    QPointer<SignInStoreReconciler> self = this;
    mPerformer(op, std::move(payload), [self, id, op](bool ok, QString error) {
        if (self) {
            self->onStepDone(id, op, ok, std::move(error));
        }
    });
}

void SignInStoreReconciler::onStepDone(unsigned int id, Operation op, bool ok, QString error)
{
    // A completion for a request that is no longer the active one (it was abandoned, and its
    // performer finished later anyway) has nothing left to drive.
    if (!mActive || mActive->id != id) {
        return;
    }

    if (mPending) {
        // A newer intent arrived while this step ran. Abandon the rest of this sequence before its
        // next operation: the newer intent describes the end state the caller wants now, and running
        // this one to completion is exactly the interleaving this class exists to prevent. Start the
        // newer one before reporting, so a completion that sets yet another intent queues behind it
        // rather than replacing it - and so mActive already holds that new request, not empty, by
        // the time the abandoned completion runs: if it calls setIntent() again and found mActive
        // empty here, that call would self-start too, colliding with the start() below.
        auto abandoned = std::move(*mActive);
        mActive.reset();
        start();
        finish(abandoned, Outcome::Superseded, op, QString());
        return;
    }

    if (mActive->removingStaleToken) {
        // The removal that follows a failed token write has answered. Either way the caller hears the
        // original failure: reaching a resume hint is damage control, not success. If the removal
        // failed too the entry is where it would have been without this step, which is the worst case
        // it can produce and no worse than not trying.
        auto failed = std::move(*mActive);
        mActive.reset();
        finish(failed, Outcome::Failed, failed.failedAt, std::move(failed.failureError));
        return;
    }

    if (!ok) {
        if (op == Operation::WriteToken) {
            // The metadata landed and the token did not. On a first save that leaves a resume hint on
            // its own, but on a rotation - the common case in production - it leaves the new metadata
            // beside the PREVIOUS token, which the server has already spent. The next connection would
            // replay that, be rejected, and fall into recovery that drops the token and, for an account
            // with no remembered provider, the whole saved sign-in with it.
            //
            // So remove the token rather than leave it: if Full cannot be reached, the nearest state
            // worth settling in is Hint, which the read path is built for. The original failure is
            // held and reported when the removal answers.
            mActive->removingStaleToken = true;
            mActive->failedAt = op;
            mActive->failureError = std::move(error);
            issue(Operation::RemoveToken, QString());
            return;
        }
        auto failed = std::move(*mActive);
        // mActive must already be empty when finish() runs: a completion that reacts to the failure
        // by calling setIntent() (a failed hint falling back to a forget) relies on finding no
        // active request here so it can self-start via setIntent()'s own "if (!mActive) { start(); }"
        // - reordering these two lines would leave that self-start unreachable.
        mActive.reset();
        finish(failed, Outcome::Failed, op, std::move(error));
        return;
    }

    ++mActive->step;
    runStep();
}

void SignInStoreReconciler::finish(Request& request, Outcome outcome, Operation failedAt, QString error)
{
    scrub(request);
    if (request.completion) {
        // Take the completion out before invoking it so a re-entrant setIntent cannot destroy the
        // std::function while it is executing.
        auto completion = std::move(request.completion);
        completion(outcome, failedAt, std::move(error));
    }
}
