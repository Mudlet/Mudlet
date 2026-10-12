#ifndef MUDLET_TSPEECHBRIDGE_H
#define MUDLET_TSPEECHBRIDGE_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "SpeechRecognizerFactory.h"

#include <QObject>
#include <QPointer>
#include <QString>

class Host;
class SpeechRecognizer;

// Speech-to-text bridge: creates the single shared recognizer on first use
// and exposes it to the Lua stt.* API. Recognizer results surface as Lua
// events; all routing and UI policy lives in packages consuming them.
class TSpeechBridge : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(TSpeechBridge)
    explicit TSpeechBridge(QObject* parent = nullptr);
    ~TSpeechBridge() override;

    // nullptr until the application has made one, and again once it is destroyed.
    static TSpeechBridge* instance() { return smpSelf; }

    // One recognizer exists at a time (docs/stt-api.md's "one recognizer per
    // client"), and backend decides what happens when one is already built:
    // Auto, or the backend already in place, keeps it - the stt.* setters pass
    // Auto on every call and must not tear down a working engine. An explicit
    // request for a different backend replaces it, which is what lets
    // stt.init() switch engines when it is handed another engine's model.
    void initSpeechRecognition(SpeechRecognizerFactory::Backend backend = SpeechRecognizerFactory::Backend::Auto);
    SpeechRecognizer* speechRecognizer() const;
    // Raise one sysSTT* event on the profile holding the microphone, or on the
    // active one when nobody holds it. Public because the stt.* bindings refuse
    // before a recognizer exists - with no engine installed there is no object
    // to emit through, and "refusals speak" has to hold there too or a consumer
    // cannot tell "no engine" from "nothing said yet".
    void raiseSpeechEvent(const QString& name, const QString& value);
    // Take the microphone for this profile, stopping whoever held it. There is
    // one recognizer for the whole application, so a second profile asking to
    // listen is a handover rather than a second session - and the profile that
    // loses it is told, since nothing else on its screen would say why its
    // microphone went quiet. Call before startListening(); on a refusal call
    // releaseMicrophone() so the claim does not outlive the session it was for.
    bool claimMicrophoneFor(Host* pHost);
    void releaseMicrophone();
    // Raise one sysSTT* event on a named profile. A refusal belongs to the
    // profile that asked for it, which is not the profile the microphone's own
    // traffic goes to once somebody else is listening.
    //
    // A sysSTTError raised while one is already being delivered is dropped: a
    // handler's own calls report their refusals through their return values, and
    // raising them would run that handler again inside itself, making the same
    // call, until Lua's C stack overflows.
    void raiseSpeechEventOn(Host* pHost, const QString& name, const QString& value);
    // Raises sysSTTCapabilitiesChanged when, and only when, what Lua reads from
    // stt.getInfo().capabilities has actually moved since it was last told.
    void announceSpeechCapabilitiesIfChanged();
    // Which profile the microphone currently belongs to, or nullptr
    Host* microphoneOwner() const;
    // Whether a recognised phrase is being handed to Lua right now. A phrase
    // that has reached its handler is not one the engine still owes anybody,
    // however busy the engine looks while that handler runs.
    bool deliveringSpeechResult() const { return mSpeechResultsBeingDelivered > 0; }
    // Called while a profile is closing, before its Host is destroyed
    void profileClosing(Host* pHost);
    // Called once the last profile has been destroyed
    void allProfilesClosed();

signals:
    // Window titles carry a marker for the profile holding the microphone
    void microphoneOwnerChanged();

private:
    inline static TSpeechBridge* smpSelf = nullptr;

    // The single shared speech recognizer (one microphone, one decoder);
    // created lazily by initSpeechRecognition()
    QPointer<SpeechRecognizer> mpSpeechRecognizer;
    // What Lua was last told stt.getInfo().capabilities are, as the event's own
    // payload. The baseline lives here rather than in the recognizer because
    // this is where Lua's view is assembled: every capability reads false while
    // no recognizer exists, so one coming into existence - or being swapped for
    // another engine - is itself a change to what getInfo() answers, and a
    // recognizer cannot notice a transition that happened before it did. Seeded
    // on first use with the all-false payload rather than left empty, so that
    // first appearance registers as the change it is (#10760).
    QString mAnnouncedSpeechCapabilities;
    // The profile that asked for the microphone, for as long as the session it
    // asked for lasts. Results belong to whoever started listening rather than
    // to whoever happens to be in front when a phrase lands: those are the same
    // profile in the ordinary case, and routing by the second one sends a
    // phrase to the wrong game in every case where they differ.
    QPointer<Host> mpMicrophoneOwner;
    // The profile whose session has just ended, until the event loop turns
    // again. An engine settles the state before it says what became of the
    // phrase that was in flight, and the release rides on the state - so
    // without this the sentence that matters most goes to whichever profile
    // happens to be in front. See raiseSpeechEvent().
    QPointer<Host> mpMicrophoneOwnerEnding;
    // How deep the delivery of a recognised phrase is - see the finalResult
    // connection in initSpeechRecognition(), and deliveringSpeechResult()
    int mSpeechResultsBeingDelivered = 0;
    // How many sysSTTError deliveries are in progress; see raiseSpeechEventOn()
    int mSpeechErrorsBeingDelivered = 0;
};

#endif // MUDLET_TSPEECHBRIDGE_H
