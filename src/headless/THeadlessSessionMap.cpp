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

#include "THeadlessSession.h"

#include "Host.h"
#include "TNullConsoleFrontend.h"
#include "TNullMapView.h"

#include <QPainter>

bool THeadlessSession::renderMap(QPainter& painter, const QSize& size)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView) {
        //: Error from a frontend with no window that was asked to draw the 2D map before a script opened one
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return false;
    }
    if (!painter.isActive()) {
        //: Error from a frontend with no window that was asked to draw the 2D map with a painter that has nothing to paint on
        mErrorString = tr("The painter is not active.");
        return false;
    }
    // T2DMap draws nothing smaller
    if (size.width() < 10 || size.height() < 10) {
        //: Error from a frontend with no window that was asked to draw the 2D map smaller than it can be drawn; %1 and %2 are the width and height asked for
        mErrorString = tr("The map is drawn at 10 by 10 pixels or more, not %1 by %2.").arg(size.width()).arg(size.height());
        return false;
    }
    pView->render(painter, size);
    return true;
}

bool THeadlessSession::mapPointer(const MapPointerEvent& event)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullMapView*>(mpHost->mapViewFrontend());
    if (!pView) {
        //: Error from a frontend with no window that was given a mouse action on the 2D map before a script opened one
        mErrorString = tr("No map is open; a script opens one, as with openMapWidget().");
        return false;
    }
    if (!pView->rendered()) {
        //: Error from a frontend with no window that was given a mouse action on the 2D map before it had drawn the map, which the action's position is in
        mErrorString = tr("The map has not been drawn yet; the position is in what renderMap() drew.");
        return false;
    }
    switch (event.type) {
    case MapPointerEvent::Type::press:
        pView->press(event.position.toPoint(), event.button, event.modifiers);
        return true;
    case MapPointerEvent::Type::doubleClick:
        if (event.button != Qt::LeftButton) {
            //: Error from a frontend with no window that was given a double-click on the 2D map with a mouse button other than the left one
            mErrorString = tr("Only a double-click of the left button acts on the map.");
            return false;
        }
        pView->doubleClick(event.position.toPoint());
        return true;
    case MapPointerEvent::Type::wheel:
        if (!pView->showsArea()) {
            //: Error from a frontend with no window that was given a turn of the mouse wheel on the 2D map while the map it last drew showed no area, as for a map with no rooms
            mErrorString = tr("The map drawn last shows no area to zoom.");
            return false;
        }
        if (!pView->wheel(event.position, event.angleDelta, event.modifiers)) {
            //: Error from a frontend with no window that was given a turn of the mouse wheel on the 2D map too small to zoom it
            mErrorString = tr("Less than half a notch of the wheel does not zoom the map.");
            return false;
        }
        return true;
    }
    return false;
}

// Gathered into one a turn, as the GUI's update() calls are into one repaint
void THeadlessSession::relayMapRepaints(Host* pHost, const unsigned int run)
{
    auto* pFrontend = dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend());
    if (!pFrontend) {
        return;
    }
    // Held by the profile, which may outlive the session
    pFrontend->mMapRepaintRequested = [self = QPointer<THeadlessSession>(this), run]() {
        if (!self || run != self->mRun || self->mMapRepaintQueuedRun == run) {
            return;
        }
        self->mMapRepaintQueuedRun = run;
        QMetaObject::invokeMethod(
                self.data(),
                [session = self.data(), run]() {
                    // A later run's cue may be queued behind this one
                    if (session->mMapRepaintQueuedRun == run) {
                        session->mMapRepaintQueuedRun = 0;
                    }
                    if (run == session->mRun) {
                        const QString profile = session->mProfileName;
                        emit session->mapRepaintRequested(profile);
                    }
                },
                Qt::QueuedConnection);
    };
}
