/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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

// The pictures a stylesheet points at are drawn into the cache folder once; what is there decides
// how they look at every screen scale and after an upgrade.

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class DesignGlyphCacheTest : public QObject
{
    Q_OBJECT

private:
    static QString twin(const QString& file, const int ratio) { return QString(file).replace(qsl(".png"), qsl("@%1x.png").arg(QString::number(ratio))); }

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void cleanupTestCase() { QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + qsl("/ui-glyphs")).removeRecursively(); }

    // QPixmap takes an @2x file as drawn at twice the size, on a 1.5x screen too
    void test_theHighDpiTwinIsWrittenAtTwiceTheSizeWhateverTheScreen()
    {
        const QString file = uiDesign::gripGlyphFile(QColor(10, 20, 30), true);
        QVERIFY2(!file.isEmpty(), "the cache could not be written");
        const QImage plain(file);
        const QImage doubled(twin(file, 2));
        QVERIFY2(!plain.isNull(), qPrintable(qsl("no picture at %1").arg(file)));
        QVERIFY2(!doubled.isNull(), qPrintable(qsl("on a %1x screen no @2x twin was written beside %2, so a high-DPI screen later reads the 1x one").arg(qApp->devicePixelRatio()).arg(file)));
        QCOMPARE(doubled.size(), plain.size() * 2);
    }

    void test_eachBuildDrawsItsOwn()
    {
        const QString file = uiDesign::gripGlyphFile(QColor(40, 50, 60), false);
        QVERIFY2(!file.isEmpty(), "the cache could not be written");
        QString build = MudletApp::scmVersion();
        build.replace(QRegularExpression(qsl("[^A-Za-z0-9._-]")), qsl("_"));
        QCOMPARE(QFileInfo(file).dir().dirName(), build);
    }

    // A twin lost from the folder, as when the cache was first written on another screen, is drawn again
    void test_aMissingTwinIsDrawnAgain()
    {
        const QString file = uiDesign::gripGlyphFile(QColor(70, 80, 90), true);
        QVERIFY(QFile::remove(twin(file, 2)));
        QCOMPARE(uiDesign::gripGlyphFile(QColor(70, 80, 90), true), file);
        QVERIFY2(QFileInfo::exists(twin(file, 2)), "a cache that already had the 1x picture never wrote the @2x one");
    }
};

#include "DesignGlyphCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(DesignGlyphCacheTest)
