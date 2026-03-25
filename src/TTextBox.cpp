/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include "TTextBox.h"

#include "Host.h"
#include "utils.h"

#include <QTextOption>

TTextBox::TTextBox(Host* pHost, const QString& name, QWidget* parent)
    : QPlainTextEdit(parent)
    , mpHost(pHost)
{
    setObjectName(qsl("textEdit_%1_%2").arg(mpHost->getName(), name));
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setFrameShape(QFrame::StyledPanel);

    QPalette palette;
    palette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    palette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);
    setPalette(palette);
}
