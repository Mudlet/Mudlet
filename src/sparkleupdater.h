/***************************************************************************
 *   Copyright (C) 2024 by Mudlet makers                                   *
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
#ifndef SPARKLEUPDATER_H
#define SPARKLEUPDATER_H
#include <QObject>

class QAction;

#ifdef __OBJC__
@class SparkleUpdaterDelegate;
#endif

class SparkleUpdater : public QObject
{
    Q_OBJECT

public:
    SparkleUpdater();
    ~SparkleUpdater();


    void checkForUpdates();

    void setAutomaticallyChecksForUpdates(bool on);
    bool automaticallyChecksForUpdates();

    void setAutomaticallyDownloadsUpdates(bool on);
    bool automaticallyDownloadsUpdates();
    // private slots:
    //     void checkForUpdates();

private:
#ifdef __OBJC__
    // for ARC tracking.
    SparkleUpdaterDelegate* _updaterDelegate;
#else
    void* _updaterDelegate;
#endif
};
#endif
