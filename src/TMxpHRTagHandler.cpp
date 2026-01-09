/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TMxpHRTagHandler.h"
#include "TMxpClient.h"

TMxpTagHandlerResult TMxpHRTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    Q_UNUSED(ctx)
    Q_UNUSED(tag)
    
    // Get the console wrap width to determine HR length
    const int width = client.getWrapWidth();
    
    // Build HR with newlines: the first \n commits the current buffered line,
    // then the dashes form the HR line, and the final \n starts a new line.
    // We inject this through insertText() which feeds it back through the MXP
    // processing pipeline (via translateToPlainText), ensuring it respects
    // the current line buffer state.
    const QString horizontalRule = qsl("\n") + QString(width, '-') + qsl("\n");
    client.insertText(horizontalRule);
    
    return MXP_TAG_HANDLED;
}
