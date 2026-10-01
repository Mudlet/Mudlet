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

#include "MudletReplay.h"

#include "Host.h"
#include "MudletApp.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

MudletReplay::MudletReplay()
{
    if (smpSelf) {
        qWarning() << "MudletReplay::MudletReplay() WARNING - a MudletReplay already exists, so self() keeps pointing at that one.";
        return;
    }
    smpSelf = this;
}

MudletReplay::~MudletReplay()
{
    if (smpSelf == this) {
        smpSelf = nullptr;
    }
}

bool MudletReplay::load(Host* pHost, const QString& fileName, QString* pErrMsg)
{
    // Refused here as well as in start(), which cTelnet only reaches after it
    // has pointed its one replay file at the new name - and that would pull the
    // file out from under a replay already running in the same profile
    if (mRunning) {
        if (pErrMsg) {
            *pErrMsg = qsl("cannot perform replay, another one seems to already be in progress; try again when it has finished.");
        } else {
            // Translated in the main window's context, where it was written
            pHost->postMessage(QCoreApplication::translate("mudlet",
                                                           "[ WARN ]  - Cannot perform replay, another one may already be in progress,\n"
                                                           "try again when it has finished."));
        }
        return false;
    }

    QString absoluteFileName;
    if (QFileInfo(fileName).isRelative()) {
        absoluteFileName = qsl("%1/%2").arg(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, pHost->getName()), fileName);
    } else {
        absoluteFileName = fileName;
    }

    return pHost->mTelnet.loadReplay(absoluteFileName, pErrMsg);
}

bool MudletReplay::start(Host* pHost)
{
    if (mRunning) {
        return false;
    }

    mRunning = true;
    mpHost = pHost;
    mSpeed = 1;
    // A null QTime would stay null through addMSecs(), so start a millisecond in
    mElapsed.setHMS(0, 0, 0, 1);
    emit signal_replayStarted();
    return true;
}

void MudletReplay::over()
{
    mRunning = false;
    mpHost = nullptr;
    emit signal_replayOver();
}

void MudletReplay::speedUp()
{
    if (!mRunning) {
        return;
    }
    mSpeed = qMin(1024, mSpeed * 2);
    emit signal_replaySpeedChanged(mSpeed);
}

void MudletReplay::speedDown()
{
    if (!mRunning) {
        return;
    }
    mSpeed = qMax(1, mSpeed / 2);
    emit signal_replaySpeedChanged(mSpeed);
}
