#ifndef MUDLET_SIDEBARTOGGLE_H
#define MUDLET_SIDEBARTOGGLE_H

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

#include "utils.h"

#include <QAbstractButton>
#include <QPointer>

namespace uiDesign {

class SidebarToggle : public QAbstractButton
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(SidebarToggle)
    // pSeam is the pane whose trailing edge the pill straddles. pParent must hold
    // both that pane and what is beside it, or either would clip the pill.
    SidebarToggle(QWidget* pSeam, QWidget* pParent);

    void setPointingLeft(const bool pointingLeft);
    [[nodiscard]] bool pointingLeft() const { return mPointingLeft; }

    // Runs by itself when either widget moves, resizes or shows; a caller only
    // needs it for any other change.
    void reposition();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
    void enterEvent(TEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QPointer<QWidget> mpSeam;
    bool mPointingLeft = true;
};

} // namespace uiDesign

#endif // MUDLET_SIDEBARTOGGLE_H
