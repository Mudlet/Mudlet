/***************************************************************************
 *   Copyright (C) 2026 by Mudlet makers                                   *
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

#include <QtTest/QtTest>

#include "dlgComposer.h"
#include "utils.h"

#include <QRegularExpression>


class AccessibleTooltipsTest : public QObject
{
    Q_OBJECT

private slots:
    void composerSaveButtonAccessibleDescriptionStripsRichTooltip()
    {
        dlgComposer composer(nullptr);

        QVERIFY(containsHtmlTag(composer.saveButton->toolTip()));
        QCOMPARE(composer.saveButton->accessibleDescription(), utils::stripHtmlTags(composer.saveButton->toolTip()));
        QVERIFY(!containsHtmlTag(composer.saveButton->accessibleDescription()));
    }

private:
    static bool containsHtmlTag(const QString& text)
    {
        static const QRegularExpression htmlTag(qsl(R"(<[/A-Za-z][^>]*>)"));
        return text.contains(htmlTag);
    }
};

QTEST_MAIN(AccessibleTooltipsTest)
#include "AccessibleTooltipsTest.moc"
