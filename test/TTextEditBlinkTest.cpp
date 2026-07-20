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

#include "TTextEdit.h"

#include <QTest>

class TTextEditBlinkTest : public QObject
{
    Q_OBJECT

private slots:
    void shouldRegisterBlinkClient_data()
    {
        QTest::addColumn<bool>("enableBlinkText");
        QTest::addColumn<bool>("hasBlinkingContentInRedrawnRegion");
        QTest::addColumn<bool>("isBlinkClientRegistered");
        QTest::addColumn<bool>("reusedCachedScreenContent");
        QTest::addColumn<bool>("expected");

        QTest::newRow("keeps registration when cached blinking content may still be visible") << true << false << true << true << true;
        QTest::newRow("unregisters when blinking is disabled") << false << true << true << true << false;
        QTest::newRow("unregisters when blinking is disabled regardless of other flags") << false << false << false << false << false;
        QTest::newRow("registers when current paint finds blinking content") << true << true << false << false << true;
        QTest::newRow("unregisters when paint did not reuse cache") << true << false << true << false << false;
        QTest::newRow("unregisters when client was not registered even with cached content") << true << false << false << true << false;
    }

    void shouldRegisterBlinkClient()
    {
        QFETCH(bool, enableBlinkText);
        QFETCH(bool, hasBlinkingContentInRedrawnRegion);
        QFETCH(bool, isBlinkClientRegistered);
        QFETCH(bool, reusedCachedScreenContent);
        QFETCH(bool, expected);

        QCOMPARE(TTextEdit::shouldRegisterBlinkClient(enableBlinkText, hasBlinkingContentInRedrawnRegion, isBlinkClientRegistered, reusedCachedScreenContent), expected);
    }
};

QTEST_GUILESS_MAIN(TTextEditBlinkTest)
#include "TTextEditBlinkTest.moc"
