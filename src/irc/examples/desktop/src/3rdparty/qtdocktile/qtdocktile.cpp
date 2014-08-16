/****************************************************************************
 *  qtdocktile.cpp
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

#include "qtdocktile.h"
#include "qtdocktile_p.h"

QtDockTile::QtDockTile(QWidget *window) :
    QObject(window), d_ptr(new QtDockTilePrivate)
{
    Q_D(QtDockTile);
    d->window = window;
}

QtDockTile::~QtDockTile()
{
}

int QtDockTile::badge() const
{
    Q_D(const QtDockTile);
    return d->badge;
}

void QtDockTile::setBadge(int badge)
{
    Q_D(QtDockTile);
    if (d->badge != badge) {
        d->badge = badge;
        d->setBadge(badge);
        emit badgeChanged(badge);
    }
}

int QtDockTile::progress() const
{
    Q_D(const QtDockTile);
    return d->progress;
}

void QtDockTile::setProgress(int progress)
{
    Q_D(QtDockTile);
    progress = qBound(0, progress, 100);
    if (d->progress != progress) {
        d->progress = progress;
        d->setProgress(progress);
        emit progressChanged(progress);
    }
}
