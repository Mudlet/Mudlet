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

#include "LuaLiteral.h"

#include <QtTest/QtTest>

#include <memory>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

/*
 * A game server controls the payload of an OSC 8 send:/prompt:/http: URI, and
 * Mudlet turns that payload into Lua source that is executed when the user
 * clicks the link. If the payload can terminate the string literal it is
 * embedded in, the remainder of the payload is executed as code. These tests
 * assert the property that matters: whatever the payload, evaluating the
 * generated literal yields the payload back and runs nothing else.
 */
class LuaLiteralTest : public QObject
{
    Q_OBJECT

private:
    // Evaluates "return <literal>" in a fresh Lua 5.1 state. Returns the
    // resulting string, or a null QString if the chunk did not compile or did
    // not produce exactly one string.
    static QString evaluate(const QString& literal)
    {
        std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), &lua_close);
        if (!state) {
            return QString();
        }

        const QByteArray chunk = QString(QLatin1String("return ") + literal).toUtf8();
        if (luaL_loadbuffer(state.get(), chunk.constData(), chunk.size(), "literal") != 0) {
            return QString();
        }
        if (lua_pcall(state.get(), 0, LUA_MULTRET, 0) != 0) {
            return QString();
        }
        if (lua_gettop(state.get()) != 1 || !lua_isstring(state.get(), -1)) {
            return QString();
        }

        size_t length = 0;
        const char* value = lua_tolstring(state.get(), -1, &length);
        return QString::fromUtf8(value, static_cast<int>(length));
    }

private slots:
    void initTestCase() {}

    void testRoundTrip_data()
    {
        QTest::addColumn<QString>("payload");

        QTest::newRow("plain command") << QStringLiteral("look");
        QTest::newRow("empty") << QStringLiteral("");
        QTest::newRow("spaces") << QStringLiteral("cast fireball at troll");
        QTest::newRow("leading newline") << QStringLiteral("\nlook");
        QTest::newRow("trailing newline") << QStringLiteral("look\n");
        QTest::newRow("embedded newline") << QStringLiteral("look\nnorth");
        QTest::newRow("single close bracket") << QStringLiteral("a]b");
        // A payload ending in "]" merges with the closer appended after it and
        // shuts the literal one character early. Ordinary MUD commands and bare
        // IPv6 URLs end this way.
        QTest::newRow("trailing close bracket") << QStringLiteral("look north]");
        QTest::newRow("bare close bracket") << QStringLiteral("]");
        QTest::newRow("ooc tag") << QStringLiteral("say [OOC]");
        QTest::newRow("inventory slot") << QStringLiteral("get sword from bag[1]");
        QTest::newRow("ipv6 url") << QStringLiteral("http://[::1]");
        QTest::newRow("trailing closer prefix level 1") << QStringLiteral("a]]b]=");
        QTest::newRow("trailing closer prefix level 2") << QStringLiteral("a]]b]=]c]==");
        QTest::newRow("level 0 breakout") << QStringLiteral("]],false) os.execute([[touch /tmp/pwned]]) --");
        QTest::newRow("level 1 breakout") << QStringLiteral("]=],false) os.execute([=[x]=]) --");
        QTest::newRow("both levels") << QStringLiteral("a]]b]=]c");
        QTest::newRow("nested open bracket") << QStringLiteral("a[[b");
        QTest::newRow("nested open bracket level 1") << QStringLiteral("a[[b]]c[=[d");
        QTest::newRow("quotes and backslashes") << QStringLiteral("say \"hi\\there\"");
        QTest::newRow("percent markers") << QStringLiteral("say %1 %2 %%");
        QTest::newRow("url with fragment") << QStringLiteral("https://example.com/a?b=c#d");
        QTest::newRow("utf8") << QStringLiteral("say éè你好");
    }

    void testRoundTrip()
    {
        QFETCH(QString, payload);

        const QString literal = LuaLiteral::quote(payload);
        const QString result = evaluate(literal);

        QVERIFY2(!result.isNull(), qPrintable(QStringLiteral("literal did not compile to a single string: %1").arg(literal)));
        QCOMPARE(result, payload);
    }

    void testBreakoutDoesNotExecute()
    {
        // The classic payload: close the literal, close the send() call, run
        // arbitrary code, comment out the tail. Building the same call shape
        // Mudlet builds must produce a chunk that assigns the payload as data.
        const QString payload = QStringLiteral("]],false) BREAKOUT = 1 --");
        const QString chunkSource = QStringLiteral("captured = %1").arg(LuaLiteral::quote(payload));

        std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), &lua_close);
        QVERIFY(state);

        const QByteArray chunk = chunkSource.toUtf8();
        QCOMPARE(luaL_loadbuffer(state.get(), chunk.constData(), chunk.size(), "chunk"), 0);
        QCOMPARE(lua_pcall(state.get(), 0, 0, 0), 0);

        lua_getglobal(state.get(), "BREAKOUT");
        QVERIFY2(lua_isnil(state.get(), -1), "payload escaped the literal and executed");
        lua_pop(state.get(), 1);

        lua_getglobal(state.get(), "captured");
        QVERIFY(lua_isstring(state.get(), -1));
        QCOMPARE(QString::fromUtf8(lua_tostring(state.get(), -1)), payload);
    }

    // The hand-picked rows above only catch payload shapes someone thought of;
    // the trailing-']' case survived review precisely because nobody did. Every
    // string over the bracket alphabet is cheap enough to just enumerate.
    void testExhaustiveBracketAlphabet()
    {
        const QList<QChar> alphabet = {QLatin1Char('['), QLatin1Char(']'), QLatin1Char('='), QLatin1Char('a')};

        QStringList current = {QString()};
        int checked = 0;
        for (int length = 1; length <= 5; ++length) {
            QStringList next;
            for (const QString& prefix : std::as_const(current)) {
                for (const QChar letter : std::as_const(alphabet)) {
                    next.append(prefix + letter);
                }
            }
            current = next;

            for (const QString& payload : std::as_const(current)) {
                const QString result = evaluate(LuaLiteral::quote(payload));
                if (result.isNull() || result != payload) {
                    QFAIL(qPrintable(QStringLiteral("payload %1 did not round-trip; literal was %2").arg(payload, LuaLiteral::quote(payload))));
                }
                ++checked;
            }
        }

        QCOMPARE(checked, 1364);
    }

    void testLevelEscalation()
    {
        // Spelling is an implementation detail, but the escalation rule is
        // worth pinning: a payload that cannot terminate level 0 must not pay
        // for a higher level.
        QVERIFY(LuaLiteral::quote(QStringLiteral("look")).startsWith(QStringLiteral("[[")));
        QVERIFY(LuaLiteral::quote(QStringLiteral("a]]b")).startsWith(QStringLiteral("[=[")));
        QVERIFY(LuaLiteral::quote(QStringLiteral("a]]b]=]c")).startsWith(QStringLiteral("[==[")));
    }
};

QTEST_MAIN(LuaLiteralTest)
#include "LuaLiteralTest.moc"
