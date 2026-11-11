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

#include "TCommandHistory.h"

#include <QFile>
#include <QSaveFile>
#include <QTextStream>

void TCommandHistory::add(QStringList& history, const QString& command)
{
    history.removeAll(command);
    if (!history.isEmpty()) {
        history[0] = command;
    } else {
        history.push_front(command);
    }
    history.push_front(QString());
}

bool TCommandHistory::read(const QString& pathFileName, QStringList& history, QString& error)
{
    QFile historyFile(pathFileName);
    if (!historyFile.exists()) {
        return true;
    }
    if (!historyFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        error = historyFile.errorString();
        return false;
    }
    QTextStream ifs(&historyFile);
    QString buffer;
    while (!ifs.atEnd() && ifs.status() == QTextStream::Ok) {
        ifs.readLineInto(&buffer);
        history.append(buffer);
    }
    if (historyFile.error() != QFileDevice::NoError) {
        error = historyFile.errorString();
        return false;
    }
    return true;
}

bool TCommandHistory::write(const QString& pathFileName, const QStringList& history, const int saveSize, QString& error)
{
    QSaveFile historyFile(pathFileName);
    if (!historyFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        error = historyFile.errorString();
        return false;
    }
    QTextStream ofs(&historyFile);
    ofs << history.mid(0, saveSize + 1).join(QChar::LineFeed);
    ofs.flush();
    if (!historyFile.commit()) {
        error = historyFile.errorString();
        return false;
    }
    return true;
}
