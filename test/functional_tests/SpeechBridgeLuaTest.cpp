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

// The stt.* Lua bindings driven from a real profile against the stand-in speech
// engine. STT_spec.lua documents the same contract, but every CI runner lacks an
// engine, so the cases that need one skip there; this is where they run.

#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "VoskRecognizer.h"
#include "VoskStubHelper.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

class SpeechBridgeLuaTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("SpeechBridgeLua-Test");
    const QString mLocalhost = qsl("localhost");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    bool mSystemEngineWins = false;

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(const QString& code) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    int luaNumber(const QString& expression) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, qsl("return %1").arg(expression).toUtf8().constData()) != 0) {
            lua_pop(L, 1);
            return -1;
        }
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // Null when the expression is nil or not a string
    QString luaString(const QString& expression) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, qsl("return %1").arg(expression).toUtf8().constData()) != 0) {
            lua_pop(L, 1);
            return QString();
        }
        const QString value = lua_type(L, -1) == LUA_TSTRING ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    bool luaTrue(const QString& expression) const { return luaString(qsl("tostring(%1)").arg(expression)) == qsl("true"); }

    // Counts a sysSTTError in _errors and keeps its payload in _lastError. While
    // _depth is above zero the handler is running inside itself, which is the
    // recursion a handler calling a refused function must not cause; _maxDepth
    // keeps the deepest it got. The handler runs body, and _errors is capped so
    // a regression ends in a failed assertion rather than a stack overflow.
    QString countErrors(const QString& body = QString()) const
    {
        return runLua(qsl("_errors, _lastError, _depth, _maxDepth = 0, nil, 0, 0\n"
                          "_handlers = _handlers or {}\n"
                          "table.insert(_handlers, registerAnonymousEventHandler('sysSTTError', function(_, message)\n"
                          "  _errors = _errors + 1\n"
                          "  _lastError = message\n"
                          "  _depth = _depth + 1\n"
                          "  if _depth > _maxDepth then _maxDepth = _depth end\n"
                          "  if _errors < 20 then %1 end\n"
                          "  _depth = _depth - 1\n"
                          "end))")
                              .arg(body));
    }

    // A directory that exists and holds no model, which the stub accepts as one.
    // Shaped like a Vosk model - the "am" subdirectory is what
    // backendForModelDir() reads - so stt.init() asks for this engine by name
    // rather than leaving Auto to answer, which on a Mac is the built-in backend
    // that takes no model path at all.
    QString stubModelDirectory() const
    {
        const QString path = QDir(mConfigDir.path()).filePath(qsl("vosk-model-stub"));
        QDir().mkpath(QDir(path).filePath(qsl("am")));
        return path;
    }

    // A folder laid out like a Vosk model, so the load reaches the backend, but
    // one the stand-in library refuses to open.
    QString unloadableStubModelDirectory() const
    {
        const QString path = QDir(mConfigDir.path()).filePath(qsl("vosk-model-stub-unloadable"));
        QDir().mkpath(QDir(path).filePath(qsl("am")));
        return path;
    }

    // A second model for the cases about one load replacing another. It has to
    // be a different directory for sameModelDirectory() to tell them apart.
    QString otherStubModelDirectory() const
    {
        const QString path = QDir(mConfigDir.path()).filePath(qsl("vosk-model-stub-other"));
        QDir().mkpath(QDir(path).filePath(qsl("am")));
        return path;
    }

    void requireStub()
    {
        if (mSystemEngineWins) {
            QSKIP("libvosk answers the bare name here, so the loader would reach it before the stand-in this case installs");
        }
        QVERIFY2(VoskStub::install(), "the stand-in engine could not be installed, so nothing below the library guard is reachable");
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        // Before anything could have loaded a stub; see VoskStub::systemEngineWins()
        mSystemEngineWins = VoskStub::systemEngineWins();

        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // A full profile rather than a bare Host: speech events go to the active
        // profile, and a profile is only active once it has a console.
        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "the profile could not be created");
        QVERIFY2(!mudlet::self()->speechRecognizer(), "a recognizer existed before any case asked for one, so its creation cannot be observed");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Everything a case left behind, after every case rather than at the end of
    // each: an assertion that fails returns before any tidying of its own.
    void cleanup()
    {
        if (!mpHost) {
            return;
        }
        runLua(qsl("for _, handler in ipairs(_handlers or {}) do killAnonymousEventHandler(handler) end\n_handlers = {}\nstt.close()"));
        VoskRecognizer::resetLibraryLoadState();
        VoskRecognizer::unloadLibraryByRequest(false);
        VoskStub::remove();
    }

    // First, while no recognizer exists: every capability reads false until one
    // does, so a recognizer coming into existence is a change a package
    // following sysSTTCapabilitiesChanged has to be told about (#10760).
    void aRecognizerComingIntoExistenceIsAnnounced()
    {
        requireStub();
        QVERIFY(runLua(qsl("_capabilityEvents = 0\n"
                           "_handlers = _handlers or {}\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTCapabilitiesChanged', function() _capabilityEvents = _capabilityEvents + 1 end))\n"
                           "local function capabilities()\n"
                           "  local c = stt.getInfo().capabilities\n"
                           "  return ('%s|%s|%s|%s'):format(tostring(c.biasing), tostring(c.grammar), tostring(c.words), tostring(c.onDevice))\n"
                           "end\n"
                           "_capabilitiesBefore = capabilities()\n"
                           "stt.setSilenceTimeout(1500)\n"
                           "_capabilitiesAfter = capabilities()"))
                        .isNull());

        QVERIFY2(luaString(qsl("_capabilitiesBefore")) != luaString(qsl("_capabilitiesAfter")),
                 "creating a recognizer on the stand-in engine did not change what getInfo() reports, so nothing here tests the announcement");
        QCOMPARE(luaNumber(qsl("_capabilityEvents")), 1);
    }

    // A sysSTTError handler that calls a function which refuses must not be run
    // again by that refusal: the refusal reaches it through the return value, and
    // raising it would start the handler inside itself, over and over.
    // No engine needed - a model path that does not exist is refused before one
    // is looked for.
    void aRefusalInsideAnErrorHandlerDoesNotRunTheHandlerAgain()
    {
        QVERIFY(countErrors(qsl("_innerOk, _innerErr = stt.init('/no/such/speech/model')")).isNull());
        QVERIFY(runLua(qsl("_ok, _err = stt.init('/no/such/speech/model')")).isNull());

        QVERIFY(luaTrue(qsl("_ok == nil")));
        QCOMPARE(luaNumber(qsl("_maxDepth")), 1);
        QCOMPARE(luaNumber(qsl("_errors")), 1);
        QVERIFY2(!luaString(qsl("_innerErr")).isEmpty(), "the call inside the handler was refused without saying why");
    }

    // The same recursion through a refusal only the engine can cause: reloading
    // the library while a model is loaded.
    void aReloadRefusedInsideAnErrorHandlerDoesNotRunTheHandlerAgain()
    {
        requireStub();
        QVERIFY(runLua(qsl("_loaded = stt.init([[%1]])").arg(stubModelDirectory())).isNull());
        QVERIFY2(luaTrue(qsl("_loaded")), "the stand-in model did not load, so the library is not in use");
        QVERIFY(countErrors(qsl("stt.reloadLibrary()")).isNull());

        QVERIFY(runLua(qsl("_ok, _err = stt.reloadLibrary()")).isNull());

        QVERIFY(luaTrue(qsl("_ok == false")));
        QCOMPARE(luaNumber(qsl("_maxDepth")), 1);
        QCOMPARE(luaNumber(qsl("_errors")), 1);
    }

    // stt.reloadLibrary() and stt.unloadLibrary() take no arguments, so a
    // refusal is the engine's doing and speaks through sysSTTError as well as in
    // the return value (#10761).
    void refusingToChangeTheLibraryInUseIsAnnounced_data()
    {
        QTest::addColumn<QString>("call");
        QTest::newRow("reload") << qsl("stt.reloadLibrary");
        QTest::newRow("unload") << qsl("stt.unloadLibrary");
    }

    void refusingToChangeTheLibraryInUseIsAnnounced()
    {
        QFETCH(QString, call);
        requireStub();
        QVERIFY(runLua(qsl("_loaded = stt.init([[%1]])").arg(stubModelDirectory())).isNull());
        QVERIFY2(luaTrue(qsl("_loaded")), "the stand-in model did not load, so the library is not in use");
        QVERIFY(countErrors().isNull());

        QVERIFY(runLua(qsl("_ok, _err = %1()").arg(call)).isNull());

        QVERIFY(luaTrue(qsl("_ok == false")));
        QCOMPARE(luaNumber(qsl("_errors")), 1);
        QCOMPARE(luaString(qsl("_lastError")), luaString(qsl("_err")));
    }

    // #10759 through the bridge: the event that lands inside stt.init()'s load is
    // the state change to ready, and a handler closing there has to turn the
    // load into a refusal that says why.
    void aReadyHandlerClosingTheBridgeMakesTheLoadFail()
    {
        requireStub();
        QVERIFY(countErrors().isNull());
        QVERIFY(runLua(qsl("_closed = false\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTStateChanged', function(_, state)\n"
                           "  if state == 'ready' and not _closed then _closed = true; stt.close() end\n"
                           "end))\n"
                           "_ok, _err = stt.init([[%1]])")
                               .arg(stubModelDirectory()))
                        .isNull());

        QVERIFY2(luaTrue(qsl("_closed")), "the handler never ran, so nothing here was re-entered");
        QVERIFY(luaTrue(qsl("_ok == nil")));
        QVERIFY2(luaString(qsl("_lastError")).contains(qsl("closed it before it could be used")), qPrintable(luaString(qsl("_lastError"))));
        QVERIFY(luaTrue(qsl("stt.initialized() == false")));
        QCOMPARE(luaString(qsl("stt.getInfo().state")), qsl("uninitialized"));
    }

    // A handler that loads the same model again, only spelt differently, has
    // replaced nothing: the model the caller asked for is the one that is
    // loaded, so the load stands. Compared as a directory rather than as text,
    // which is the only way a trailing separator or a symlink reads as the same
    // model.
    void aReadyHandlerReloadingTheSameModelLeavesTheLoadStanding()
    {
        requireStub();
        const QString model = stubModelDirectory();
        QVERIFY(runLua(qsl("_reentered = false\n"
                           "_handlers = _handlers or {}\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTStateChanged', function(_, state)\n"
                           "  if state == 'ready' and not _reentered then _reentered = true; stt.init([[%1/]]) end\n"
                           "end))\n"
                           "_ok, _err = stt.init([[%2]])")
                               .arg(model, model))
                        .isNull());

        QVERIFY2(luaTrue(qsl("_reentered")), "the handler never ran, so nothing here was re-entered");
        QVERIFY2(luaTrue(qsl("_ok")), qPrintable(qsl("a reload of the same model was taken for a replacement: %1").arg(luaString(qsl("_err")))));
        QVERIFY(luaTrue(qsl("stt.initialized()")));
    }

    // A model load ends a session that was under way and says so, and that report
    // reaches Lua while the load is still running. A handler that loads a model
    // of its own from there is told it succeeded - and it did - so the load it
    // interrupted must not free that model and install its own behind it, with
    // both calls answering true.
    //
    // Both reports reach a handler at this same point, and neither refused the
    // outer call before this guard: endSessionForModelLoad() settles the state to
    // Ready before emitting the error, and mudlet.cpp raises
    // sysSTTStateChanged("ready") synchronously, so a state-change handler
    // re-enters here exactly as a sysSTTError one does. The refusal that already
    // existed is a different case - a handler on the post-commit "ready" from
    // settleAfterModelLoad(), which STT_spec.lua covers.
    void aModelLoadedFromTheLostSessionReportSurvives()
    {
        requireStub();
        const QString model = stubModelDirectory();
        const QString handlerModel = otherStubModelDirectory();

        QVERIFY(runLua(qsl("_ok, _err = stt.init([[%1]])").arg(model)).isNull());
        QVERIFY2(luaTrue(qsl("_ok")), qPrintable(qsl("the stand-in model did not load: %1").arg(luaString(qsl("_err")))));
        QVERIFY(runLua(qsl("_started = stt.start()")).isNull());
        if (!luaTrue(qsl("_started"))) {
            QSKIP("no session could be started here, so no load can interrupt one");
        }

        QVERIFY(runLua(qsl("_handlerRan = false\n"
                           "_handlerOk = nil\n"
                           "_handlers = _handlers or {}\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTError', function(_, message)\n"
                           "  if not _handlerRan and message:find('stopped the listening session') then\n"
                           "    _handlerRan = true\n"
                           "    _handlerOk = stt.init([[%1]])\n"
                           "  end\n"
                           "end))\n"
                           "_outerOk, _outerErr = stt.init([[%2]])")
                               .arg(handlerModel, model))
                        .isNull());

        QVERIFY2(luaTrue(qsl("_handlerRan")), "the handler never ran, so nothing here was re-entered");
        QVERIFY2(luaTrue(qsl("_handlerOk")), "the handler's own load was refused, so this case is not about the load that interrupted it");
        QCOMPARE(luaString(qsl("stt.getInfo().modelPath")), handlerModel);
        QVERIFY2(!luaTrue(qsl("_outerOk")), qPrintable(qsl("both loads answered true - the model in place is '%1'").arg(luaString(qsl("stt.getInfo().modelPath")))));
        QVERIFY2(luaString(qsl("_outerErr")).contains(qsl("replaced it with another")),
                 qPrintable(qsl("the outer load did not say a handler had replaced its model: \"%1\"").arg(luaString(qsl("_outerErr")))));
    }

    // A handler that only changes a setting has replaced nothing, so the load it
    // interrupted must carry on. This is how the shipped sherpa backend applies
    // stt.setSensitivity() and stt.setVocabulary(): both rebuild the model
    // already in place, through the same load path that commits a replacement.
    // Counting that rebuild made the outer stt.init() stand down for a
    // replacement that never happened, leaving the old model loaded and the
    // caller told a handler had swapped it. Reproduced here as a handler
    // re-loading the model already in place, which is the same commit through
    // the same counter - the stand-in library is Vosk's, and the sherpa rebuild
    // has no in-tree stub.
    void aHandlerReloadingTheSameModelDoesNotStopTheLoadItInterrupted()
    {
        requireStub();
        const QString model = stubModelDirectory();
        const QString outerModel = otherStubModelDirectory();

        QVERIFY(runLua(qsl("_ok, _err = stt.init([[%1]])").arg(model)).isNull());
        QVERIFY2(luaTrue(qsl("_ok")), qPrintable(qsl("the stand-in model did not load: %1").arg(luaString(qsl("_err")))));
        QVERIFY(runLua(qsl("_started = stt.start()")).isNull());
        if (!luaTrue(qsl("_started"))) {
            QSKIP("no session could be started here, so no load can interrupt one");
        }

        // The handler asks for the model that is already loaded, which is what a
        // setting change amounts to underneath.
        QVERIFY(runLua(qsl("_handlerRan = false\n"
                           "_handlerOk = nil\n"
                           "_handlers = _handlers or {}\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTError', function(_, message)\n"
                           "  if not _handlerRan and message:find('stopped the listening session') then\n"
                           "    _handlerRan = true\n"
                           "    _handlerOk = stt.init([[%1]])\n"
                           "  end\n"
                           "end))\n"
                           "_outerOk, _outerErr = stt.init([[%2]])")
                               .arg(model, outerModel))
                        .isNull());

        QVERIFY2(luaTrue(qsl("_handlerRan")), "the handler never ran, so nothing here was re-entered");
        QVERIFY2(luaTrue(qsl("_handlerOk")), "the handler's own load was refused, so this case is not about the load that interrupted it");
        QVERIFY2(luaTrue(qsl("_outerOk")),
                 qPrintable(qsl("the load stood down for a rebuild that replaced nothing: \"%1\", and the model in place is '%2'")
                                    .arg(luaString(qsl("_outerErr")), luaString(qsl("stt.getInfo().modelPath")))));
        QCOMPARE(luaString(qsl("stt.getInfo().modelPath")), outerModel);
    }

    // The other half of the case above. A handler whose own load fails has
    // replaced nothing, so the load it interrupted must carry on and install
    // the model it was asked for. Counting a load as a replacement from the
    // moment it began, rather than once it committed, had the interrupted load
    // stand down here too - leaving neither model installed, and sttInit()
    // blaming a replacement that never happened.
    //
    // The outer call asks for a different model from the one already loaded,
    // or a load that wrongly stood down would leave an equal model in place and
    // sttInit() would find nothing amiss.
    void aHandlersFailedLoadDoesNotStopTheLoadItInterrupted()
    {
        requireStub();
        const QString model = stubModelDirectory();
        const QString alreadyLoaded = otherStubModelDirectory();
        const QString unloadable = unloadableStubModelDirectory();

        QVERIFY(runLua(qsl("_ok, _err = stt.init([[%1]])").arg(alreadyLoaded)).isNull());
        QVERIFY2(luaTrue(qsl("_ok")), qPrintable(qsl("the stand-in model did not load: %1").arg(luaString(qsl("_err")))));
        QVERIFY(runLua(qsl("_started = stt.start()")).isNull());
        if (!luaTrue(qsl("_started"))) {
            QSKIP("no session could be started here, so no load can interrupt one");
        }

        QVERIFY(runLua(qsl("_failedRan = false\n"
                           "_failedOk = nil\n"
                           "_handlers = _handlers or {}\n"
                           "table.insert(_handlers, registerAnonymousEventHandler('sysSTTError', function(_, message)\n"
                           "  if not _failedRan and message:find('stopped the listening session') then\n"
                           "    _failedRan = true\n"
                           "    _failedOk = stt.init([[%1]])\n"
                           "  end\n"
                           "end))\n"
                           "_outerOk, _outerErr = stt.init([[%2]])")
                               .arg(unloadable, model))
                        .isNull());

        QVERIFY2(luaTrue(qsl("_failedRan")), "the handler never ran, so nothing here was re-entered");
        QVERIFY2(!luaTrue(qsl("_failedOk")), "the handler's load of an unloadable model succeeded, so this case is not about a failed load");
        QVERIFY2(luaTrue(qsl("_outerOk")), qPrintable(qsl("the load was abandoned for a handler load that failed: \"%1\"").arg(luaString(qsl("_outerErr")))));
        QCOMPARE(luaString(qsl("stt.getInfo().modelPath")), model);
    }

    // Reloading a library that is not there has nothing to report success about,
    // and a false with nothing else leaves the caller no way to tell why.
    void aReloadThatFindsNoLibrarySaysWhy()
    {
        if (mSystemEngineWins) {
            QSKIP("a speech engine library is installed here, so a reload finds one");
        }
        QVERIFY(countErrors().isNull());

        QVERIFY(runLua(qsl("_ok, _err = stt.reloadLibrary()")).isNull());

        if (!luaTrue(qsl("_ok == false"))) {
            QSKIP("speech is available here without a library to find - the built-in backend needs none - so the reload had something to report");
        }
        QVERIFY2(!luaString(qsl("_err")).isEmpty(), "the reload failed without saying why");
        QCOMPARE(luaNumber(qsl("_errors")), 1);
    }
};

#include "SpeechBridgeLuaTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechBridgeLuaTest)
