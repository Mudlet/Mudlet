/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022, 2024 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2022-2023 by Lecker Kebap - Leris@mudlet.org            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// mapper-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

#include "TLuaInterpreter.h"

#include "EAction.h"
#include "Host.h"
#include "TAlias.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TFlipButton.h"
#include "TForkedProcess.h"
#include "TLabel.h"
#include "TMapLabel.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif

#include <limits>
#include <math.h>

#include "pre_guard.h"
#include <QtConcurrent>
#include <QCollator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTableWidget>
#include <QToolTip>
#include <QFileInfo>
#include <QMovie>
#include <QVector>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB
#include "post_guard.h"

#ifdef QT_TEXTTOSPEECH_LIB
QPointer<QTextToSpeech> speechUnit;
QVector<QString> speechQueue;
bool bSpeechBuilt;
bool bSpeechQueueing;
int speechState = QTextToSpeech::State::Ready;
QString speechCurrent;

// BackendError was renamed to Error in Qt6
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static const QTextToSpeech::State TEXT_TO_SPEECH_ERROR_STATE = QTextToSpeech::State::BackendError;
#else
static const QTextToSpeech::State TEXT_TO_SPEECH_ERROR_STATE = QTextToSpeech::State::Error;
#endif

// No documentation available in wiki - internal function
void TLuaInterpreter::ttsBuild()
{
    if (bSpeechBuilt) {
        return;
    }

    speechUnit = new QTextToSpeech();
    bSpeechBuilt = true;
    bSpeechQueueing = false;

    connect(speechUnit, &QTextToSpeech::stateChanged, &TLuaInterpreter::ttsStateChanged);
    return;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSkip
int TLuaInterpreter::ttsSkip(lua_State* L)
{
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->stop();

    return 0;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::ttsStateChanged(QTextToSpeech::State state)
{
    if (state != speechState) {
        speechState = state;
        TEvent event {};
        switch (state) {
        case QTextToSpeech::State::Paused:
            event.mArgumentList.append(QLatin1String("ttsSpeechPaused"));
            break;
        case QTextToSpeech::State::Speaking:
            event.mArgumentList.append(QLatin1String("ttsSpeechStarted"));
            break;
        case TEXT_TO_SPEECH_ERROR_STATE:
            event.mArgumentList.append(QLatin1String("ttsSpeechError"));
            break;
        case QTextToSpeech::State::Ready:
            event.mArgumentList.append(QLatin1String("ttsSpeechReady"));
            break;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        // The Qt documentation is not clear exactly when this change was made
        // (it says nothing!) but it isn't present in 5.15.13 but is in 6.6.2
        case QTextToSpeech::State::Synthesizing:
            event.mArgumentList.append(QLatin1String("ttsSpeechSynthesizing"));
            break;
#endif
        }
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (state == QTextToSpeech::Speaking) {
            event.mArgumentList.append(speechCurrent);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }

        mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);
    }

    if (state != QTextToSpeech::State::Ready || speechQueue.empty()) {
        bSpeechQueueing = false;
        return;
    }

    QString textToSay;
    textToSay = speechQueue.takeFirst();

    speechUnit->say(textToSay);
    speechCurrent = textToSay;

    return;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsClearQueue
int TLuaInterpreter::ttsClearQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (lua_gettop(L) > 0) {
        int index = getVerifiedInt(L, __func__, 1, "index");
        index--;
        if (index < 0 || index >= speechQueue.size()) {
            return warnArgumentValue(L, __func__, qsl("index %1 out of bounds for queue size %2").arg(index + 1, speechQueue.size()));
        }

        speechQueue.remove(index);
        return 0;
    }

    speechQueue.clear();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetCurrentLine
int TLuaInterpreter::ttsGetCurrentLine(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (speechUnit->state() == QTextToSpeech::State::Ready) {
        return warnArgumentValue(L, __func__, "not speaking any text");
    } else if (speechUnit->state() == TEXT_TO_SPEECH_ERROR_STATE) {
        return warnArgumentValue(L, __func__, "error with the computer's TTS engine");
    }

    lua_pushstring(L, speechCurrent.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetCurrentVoice
int TLuaInterpreter::ttsGetCurrentVoice(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    const QString currentVoice = speechUnit->voice().name();
    lua_pushstring(L, currentVoice.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetPitch
int TLuaInterpreter::ttsGetPitch(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    lua_pushnumber(L, speechUnit->pitch());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetQueue
int TLuaInterpreter::ttsGetQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (lua_gettop(L) > 0) {
        int index = getVerifiedInt(L, __func__, 1, "index");
        index--;
        if (index < 0 || index > speechQueue.size()) {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushstring(L, speechQueue.at(index).toUtf8().constData());
        return 1;
    }

    lua_newtable(L);

    for (int i = 0; i < speechQueue.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, speechQueue.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetRate
int TLuaInterpreter::ttsGetRate(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    lua_pushnumber(L, speechUnit->rate());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetState
int TLuaInterpreter::ttsGetState(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    switch (speechUnit->state()) {
    case QTextToSpeech::State::Ready:
        lua_pushstring(L, "ttsSpeechReady");
        break;
    case QTextToSpeech::State::Paused:
        lua_pushstring(L, "ttsSpeechPaused");
        break;
    case QTextToSpeech::State::Speaking:
        lua_pushstring(L, "ttsSpeechStarted");
        break;
    case TEXT_TO_SPEECH_ERROR_STATE:
        lua_pushstring(L, "ttsSpeechError");
        break;
    default:
        lua_pushstring(L, "ttsUnknownState");
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetVoices
int TLuaInterpreter::ttsGetVoices(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    QVector<QVoice> const speechVoices = speechUnit->availableVoices();
    int i = 0;
    lua_newtable(L);
    for (const QVoice& voice : speechVoices) {
        lua_pushnumber(L, ++i);
        lua_pushstring(L, voice.name().toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetVolume
int TLuaInterpreter::ttsGetVolume(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    lua_pushnumber(L, speechUnit->volume());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsPause
int TLuaInterpreter::ttsPause(lua_State* L)
{
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->pause();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsQueue
int TLuaInterpreter::ttsQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    QString inputText = getVerifiedString(L, __func__, 1, "input").trimmed();
    if (inputText.isEmpty()) { // there's nothing more to say. discussion: https://github.com/Mudlet/Mudlet/issues/4688
        return warnArgumentValue(L, __func__, qsl("skipped empty text to speak (TTS)"));
    }

    std::vector<QString> const dontSpeak = {"<", ">", "&lt;", "&gt;"}; // discussion: https://github.com/Mudlet/Mudlet/issues/4689
    for (const QString& dropThis : dontSpeak) {
        if (inputText.contains(dropThis)) {
            inputText.replace(dropThis, QString());
            if (mudlet::smDebugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA: removed angle-shaped brackets (<>) from text to speak (TTS)\n" >> &host;
            }
        }
    }

    int index;
    if (lua_gettop(L) > 1) {
        index = getVerifiedInt(L, __func__, 2, "index");
        index--;
        if (index < 0) {
            index = 0;
        }
        if (index > speechQueue.size()) {
            index = speechQueue.size();
        }
    } else {
        index = speechQueue.size();
    }

    speechQueue.insert(index, inputText);

    TEvent event {};
    Host& host = getHostFromLua(L);
    event.mArgumentList.append(QLatin1String("ttsSpeechQueued"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(inputText);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(index));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    host.raiseEvent(event);

    if (speechQueue.size() == 1 && speechUnit->state() == QTextToSpeech::State::Ready && !bSpeechQueueing) {
        bSpeechQueueing = true;
        TLuaInterpreter::ttsStateChanged(speechUnit->state());
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsResume
int TLuaInterpreter::ttsResume(lua_State* L)
{
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->resume();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSpeak
int TLuaInterpreter::ttsSpeak(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    QString textToSay = getVerifiedString(L, __func__, 1, "text to say").trimmed();
    if (textToSay.isEmpty()) { // there's nothing more to say. discussion: https://github.com/Mudlet/Mudlet/issues/4688
        return warnArgumentValue(L, __func__, qsl("skipped empty text to speak (TTS)"));
    }

    std::vector<QString> const dontSpeak = {"<", ">", "&lt;", "&gt;"}; // discussion: https://github.com/Mudlet/Mudlet/issues/4689
    for (const QString& dropThis : dontSpeak) {
        if (textToSay.contains(dropThis)) {
            textToSay.replace(dropThis, QString());
            if (mudlet::smDebugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA: removed angle-shaped brackets (<>) from text to speak (TTS)\n" >> &host;
            }
        }
    }

    speechUnit->say(textToSay);
    speechCurrent = textToSay;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetPitch
int TLuaInterpreter::ttsSetPitch(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    double pitch = getVerifiedDouble(L, __func__, 1, "pitch");

    if (pitch > 1.0) {
        pitch = 1.0;
    }

    if (pitch < -1.0) {
        pitch = -1.0;
    }

    speechUnit->setPitch(pitch);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsPitchChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(pitch));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetRate
int TLuaInterpreter::ttsSetRate(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    double rate = getVerifiedDouble(L, __func__, 1, "rate");

    if (rate > 1.0) {
        rate = 1.0;
    }

    if (rate < -1.0) {
        rate = -1.0;
    }

    speechUnit->setRate(rate);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsRateChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(rate));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVolume
int TLuaInterpreter::ttsSetVolume(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    double volume = getVerifiedDouble(L, __func__, 1, "volume");

    if (volume > 1.0) {
        volume = 1.0;
    }

    if (volume < 0.0) {
        volume = 0.0;
    }

    speechUnit->setVolume(volume);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsVolumeChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(volume));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVoiceByIndex
int TLuaInterpreter::ttsSetVoiceByIndex(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    int index = getVerifiedInt(L, __func__, 1, "voice as index number");
    index--;

    QVector<QVoice> speechVoices = speechUnit->availableVoices();
    if (index < 0 || index >= speechVoices.size()) {
        lua_pushboolean(L, false);
        return 1;
    }

    speechUnit->setVoice(speechVoices.at(index));

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsVoiceChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(speechVoices[index].name());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVoiceByName
int TLuaInterpreter::ttsSetVoiceByName(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    const QString nextVoice = getVerifiedString(L, __func__, 1, "voice");

    QVector<QVoice> const speechVoices = speechUnit->availableVoices();
    for (auto voice : speechVoices) {
        if (voice.name() == nextVoice) {
            speechUnit->setVoice(voice);
            lua_pushboolean(L, true);

            TEvent event {};
            event.mArgumentList.append(QLatin1String("ttsVoiceChanged"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            event.mArgumentList.append(voice.name());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

            return 1;
        }
    }

    lua_pushboolean(L, false);
    return 1;
}


#endif // QT_TEXTTOSPEECH_LIB
