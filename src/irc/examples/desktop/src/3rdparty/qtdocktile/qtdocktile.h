/****************************************************************************
 *  qtdocktile.h
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

#ifndef QTDOCKICON_H
#define QTDOCKICON_H

#include <QObject>

class QtDockTilePrivate;

class QtDockTile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable)
    Q_PROPERTY(int badge READ badge WRITE setBadge NOTIFY badgeChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

public:
    explicit QtDockTile(QWidget *window);
    virtual ~QtDockTile();

    static bool isAvailable();

    int badge() const;
    void setBadge(int badge);

    int progress() const;
    void setProgress(int percent);

signals:
    void badgeChanged(int value);
    void progressChanged(int progress);

protected:
    Q_DECLARE_PRIVATE(QtDockTile)
    QScopedPointer<QtDockTilePrivate> d_ptr;
};

#endif // QTDOCKICON_H
