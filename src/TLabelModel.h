#ifndef MUDLET_TLABELMODEL_H
#define MUDLET_TLABELMODEL_H

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

#include <QColor>
#include <QPointer>
#include <QSet>
#include <QString>

class Host;

// A label's widget-free state, so the Widgets-free core can hold a named label without a QLabel.
// Owned by its TLabel, whose same-named reference members alias it; TWindowRegistry indexes
// these by name but does NOT own them.
struct TLabelModel
{
    // Defined out of line: mpHost is a QPointer, which needs Host complete.
    TLabelModel(Host* pHost, const QString& name);
    // Frees the callbacks' Lua registry indexes.
    ~TLabelModel();

    // A copy would free the same Lua registry indexes twice. This also suppresses the implicit moves.
    TLabelModel(const TLabelModel&) = delete;
    TLabelModel& operator=(const TLabelModel&) = delete;

    // Each releases the Lua registry index it replaces.
    void setClick(const int func);
    void setDoubleClick(const int func);
    void setRelease(const int func);
    void setMove(const int func);
    void setWheel(const int func);
    void setEnter(const int func);
    void setLeave(const int func);

    // A QPointer because Host and view are torn down in either order: quitting
    // destroys every Host before the labels' deferred deletes run.
    QPointer<Host> mpHost;
    QString mName;
    int mClickFunction = 0;
    int mDoubleClickFunction = 0;
    int mReleaseFunction = 0;
    int mMoveFunction = 0;
    int mWheelFunction = 0;
    int mEnterFunction = 0;
    int mLeaveFunction = 0;
    QString mLinkColor;
    QString mLinkVisitedColor;
    bool mLinkUnderline = true;
    QSet<QString> mVisitedLinks;
    // What setBackgroundColor() was last given; a label restyled with its own
    // background-color stylesheet paints something else.
    QColor mBackgroundColor;
    // The tint and the transforms of an SVG background are properties of the
    // label that outlive any particular image, so they belong with the rest of
    // its widget-free state; the renderer that draws them stays with the widget.
    QColor mSvgTintColor;
    double mSvgRotation = 0.0;
    double mSvgShearX = 0.0;
    double mSvgShearY = 0.0;

private:
    void releaseFunc(const int existingFunction, const int newFunction);
};

#endif // MUDLET_TLABELMODEL_H
