/****************************************************************************
 *  windowstaskbar.cpp
 *
 *  Copyright (c) 2011 by Sidorov Aleksey <gorthauer87@ya.ru>
 *
 ***************************************************************************
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
*****************************************************************************/

#include <qt_windows.h>
#include "winutils.h"

#include "qtdocktile.h"
#include "qtdocktile_p.h"
#include <QApplication>
#include <QSysInfo>
#include <QPainter>

static ITaskbarList3 *windowsTaskBar()
{
    ITaskbarList3 *taskbar = 0;
    if (S_OK != CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&taskbar))
        return 0;
    return taskbar;
}

static void setOverlayIcon(HWND winId, HICON icon)
{
    ITaskbarList3 *taskbar = windowsTaskBar();
    if (!taskbar)
        return;
    taskbar->SetOverlayIcon(winId, icon, L"No description");
    taskbar->Release();
}

static QPixmap createBadge(int badge, const QPalette &palette)
{
    QPixmap pixmap(16, 16);
    QRect rect = pixmap.rect();
    rect.adjust(1, 1, -1, -1);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(palette.toolTipBase());

    QPen pen = painter.pen();
    pen.setColor(palette.color(QPalette::ToolTipText));
    painter.setPen(pen);

    QString label = QFontMetrics(painter.font()).elidedText(QString::number(badge), Qt::ElideMiddle, rect.width());
    painter.drawRoundedRect(rect, 5, 5);
    painter.drawText(rect, Qt::AlignCenter | Qt::TextSingleLine, label);
    return pixmap;
}

bool QtDockTile::isAvailable()
{
    return QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7;
}

void QtDockTilePrivate::setBadge(int badge)
{
    if (badge > 0) {
        QPixmap pixmap = createBadge(badge, window->palette());
        setOverlayIcon(window->winId(), pixmap.toWinHICON());
    } else
        setOverlayIcon(window->winId(), 0);
}

void QtDockTilePrivate::setProgress(int progress)
{
    ITaskbarList3 *taskbar = windowsTaskBar();
    if (!taskbar)
        return;
    taskbar->HrInit();
    taskbar->SetProgressValue(window->winId(), progress, 100);
    taskbar->SetProgressState(window->winId(), progress ? TBPF_NORMAL : TBPF_NOPROGRESS);
    taskbar->Release();
}
