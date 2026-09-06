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

// The per-label data model: the slice of a label's state that needs no widget.
// Holding it apart is what lets the core (Host) hold a handle to a named label
// without holding a QLabel, which is the point of the Widgets-free core (#8681).
//
// A label is view-owned, so its model is owned by its TLabel (reached through
// TLabel::model()); the widget exposes same-named members as references aliasing
// the model, so widget-side code reads and writes the model directly. The core's
// window registry indexes these models by name and does NOT own them - see
// TWindowRegistry.
struct TLabelModel
{
    // Defined out of line: mpHost is a QPointer, which needs Host complete.
    TLabelModel(Host* pHost, const QString& name);
    // Frees the callback registry indexes, so a label that is destroyed without
    // its callbacks having been replaced does not leak them.
    ~TLabelModel();

    // A copy would leave two models claiming the same Lua registry indexes, and
    // free them twice. Deleting the copy operations suppresses the implicit move
    // ones too.
    TLabelModel(const TLabelModel&) = delete;
    TLabelModel& operator=(const TLabelModel&) = delete;

    // Each releases the index it is replacing, so a callback set twice does not
    // strand the first function in the Lua registry.
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

private:
    void releaseFunc(const int existingFunction, const int newFunction);
};

#endif // MUDLET_TLABELMODEL_H
