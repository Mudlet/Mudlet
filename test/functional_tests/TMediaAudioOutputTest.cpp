/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QPointer>
#include <QtTest/QtTest>

#include "TMedia.h"
#include "TMediaData.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// TMediaPlayer::refreshAudioOutput() is what a playing player goes through when the system's
// audio output device changes - TMedia::refreshAudioDevices() skips the stopped ones. There is
// no Lua entry point to it, mudlet calls refreshAudioDevices() off QMediaDevices'
// audioOutputsChanged, so this is a functional test rather than a spec.
class TMediaAudioOutputTest : public QObject
{
    Q_OBJECT

private slots:
    // QMediaPlayer::setAudioOutput() does not take ownership, so a device change has to leave
    // the player owning exactly its replacement output: unparented, the replacement is owned by
    // nobody and outlives the player, and unreleased, the old one stays a second child that is
    // already queued for deletion (#9237).
    void test_aDeviceChangeLeavesThePlayerOwningOnlyTheNewAudioOutput()
    {
        TMediaData mediaData{};
        QPointer<QAudioOutput> displaced;
        QPointer<QAudioOutput> replacement;

        {
            TMediaPlayer player(nullptr, mediaData);
            displaced = player.mediaPlayer()->audioOutput();
            QVERIFY(displaced);

            player.refreshAudioOutput();

            const auto owned = player.mediaPlayer()->findChildren<QAudioOutput*>(Qt::FindDirectChildrenOnly);
            QCOMPARE(owned.size(), 1);
            QVERIFY2(owned.constFirst() != displaced, "The player still owns the output it was refreshed away from, which is already queued for deletion.");
            replacement = owned.constFirst();
            QVERIFY2(player.mediaPlayer()->audioOutput() == replacement, "The player reports no audio output after a device change, so volume and mute stop reaching it (#11012).");

            // Deferred, not immediate: #9237 also replaced a manual delete of an output the
            // backend may still be holding, and only the old one still being alive at this
            // point tells the two apart.
            QVERIFY2(!displaced.isNull(), "The displaced audio output was deleted outright instead of being handed to deleteLater().");

            QTest::qWait(1ms); // runs the deleteLater() the displaced output was handed to
            QVERIFY2(displaced.isNull(), "The displaced audio output was never deleted, so every device change leaves one behind.");
        }

        QVERIFY2(replacement.isNull(), "The audio output a device change installed outlived the player that was using it, so nothing will ever delete it.");
    }

    // A second device change reads the volume and mute to carry over from the output the
    // first one installed, so that output has to still be the one the player reports (#11012).
    void test_repeatedDeviceChangesKeepVolumeMuteAndASingleAudioOutput()
    {
        TMediaData mediaData{};
        TMediaPlayer player(nullptr, mediaData);
        QAudioOutput* original = player.mediaPlayer()->audioOutput();
        QVERIFY(original);
        original->setVolume(0.25f);
        original->setMuted(true);

        for (int change = 1; change <= 3; ++change) {
            player.refreshAudioOutput();
            QTest::qWait(1ms);

            QAudioOutput* current = player.mediaPlayer()->audioOutput();
            QVERIFY2(current, qPrintable(qsl("The player reports no audio output after device change %1.").arg(change)));
            QCOMPARE(current->volume(), 0.25f);
            QVERIFY(current->isMuted());
            QCOMPARE(player.mediaPlayer()->findChildren<QAudioOutput*>(Qt::FindDirectChildrenOnly).size(), 1);
        }
    }
};

#include "TMediaAudioOutputTest.moc"
MUDLET_GROUPED_TEST_MAIN(TMediaAudioOutputTest)
