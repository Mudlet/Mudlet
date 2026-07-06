#ifndef MUDLET_TFEATURECALLOUT_H
#define MUDLET_TFEATURECALLOUT_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include <QPointer>
#include <QSet>
#include <QWidget>

// A small dismissible balloon anchored to another widget, used to point out
// a newly added part of the interface without taking over the screen.
//
// Call maybeShow() from the code that owns the anchor widget, at the moment
// the anchor becomes available; it decides whether this player should still
// be told about the feature. The balloon goes away for good once the player
// engages - clicking "Got it" or the anchor itself - and gives up on its own
// after enough ignored appearances.
class TFeatureCallout : public QWidget
{
    Q_OBJECT

public:
    TFeatureCallout(const QString& featureId, QWidget* pAnchor, const QString& title, const QString& body);

    static void maybeShow(const QString& featureId, QWidget* pAnchor, const QString& title, const QString& body);

    void showAnchored();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void reposition();
    void markDismissed();

    QString mFeatureId;
    QPointer<QWidget> mpAnchor;
    // Read out to screen readers when the balloon appears
    QString mAnnouncement;
    // Horizontal position of the arrow tip within the balloon, kept in sync
    // with wherever the anchor currently sits on screen
    int mArrowX = 0;
    // The balloon prefers hanging below the anchor, but flips above it when
    // there is not enough screen space underneath
    bool mArrowOnTop = true;

    // Features already pointed out this session, also used to keep several
    // freshly shipped features from ganging up on the player at once
    inline static QSet<QString> smSessionShown;
};

#endif // MUDLET_TFEATURECALLOUT_H
