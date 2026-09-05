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

// Everything Mudlet sends that is not itself a telnet command goes through
// cTelnet::escapeIac(), and a 0xFF byte it fails to double is read by the game
// server as the start of one - so the rest of that line is eaten, or worse,
// answered as a negotiation.

#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "ctelnet.h"

#include <string>

namespace {
// Spelled out rather than taken from TN_IAC so the test pins the wire value.
constexpr char iac = '\xff';

QByteArray asBytes(const std::string& text)
{
    return QByteArray::fromStdString(text);
}
} // namespace

class TelnetIacEscapingTest : public QObject
{
    Q_OBJECT

private slots:
    void leavesDataWithoutIacUntouched()
    {
        const std::string input("say hello\r\n");
        QCOMPARE(asBytes(cTelnet::escapeIac(input)), asBytes(input));
    }

    void doublesASingleIac()
    {
        const std::string input = std::string("a") + iac + std::string("b");
        const std::string expected = std::string("a") + iac + iac + std::string("b");
        QCOMPARE(asBytes(cTelnet::escapeIac(input)), asBytes(expected));
    }

    void doublesEachOfAdjacentIacs()
    {
        const std::string input = std::string("a") + iac + iac + std::string("b");
        const std::string expected = std::string("a") + iac + iac + iac + iac + std::string("b");
        QCOMPARE(asBytes(cTelnet::escapeIac(input)), asBytes(expected));
    }

    void doublesATrailingIac()
    {
        const std::string input = std::string("a") + iac;
        const std::string expected = std::string("a") + iac + iac;
        QCOMPARE(asBytes(cTelnet::escapeIac(input)), asBytes(expected));
    }

    void handlesAnEmptyString() { QCOMPARE(asBytes(cTelnet::escapeIac(std::string())), QByteArray()); }

    void keepsEmbeddedNulBytes()
    {
        const std::string input = std::string("a\0", 2) + iac + std::string("\0b", 2);
        const std::string expected = std::string("a\0", 2) + iac + iac + std::string("\0b", 2);
        QCOMPARE(asBytes(cTelnet::escapeIac(input)), asBytes(expected));
        QCOMPARE(cTelnet::escapeIac(input).size(), expected.size());
    }

    // The framing bytes of a subnegotiation are telnet commands, so escaping the
    // finished message instead of its payload doubles the leading IAC and the
    // server reads a literal 0xFF followed by raw text - no 102 subnegotiation
    // reaches it at all.
    void framesTheChannel102PayloadWithoutEscapingTheFraming()
    {
        const std::string expected = std::string() + iac + '\xfa' + '\x66' + "ab" + iac + '\xf0';
        QCOMPARE(asBytes(cTelnet::buildChannel102Message("ab")), asBytes(expected));
    }

    void escapesAnIacInsideTheChannel102Payload()
    {
        const std::string expected = std::string() + iac + '\xfa' + '\x66' + iac + iac + 'b' + iac + '\xf0';
        QCOMPARE(asBytes(cTelnet::buildChannel102Message(std::string() + iac + 'b')), asBytes(expected));
    }
};

#include "TelnetIacEscapingTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetIacEscapingTest)
