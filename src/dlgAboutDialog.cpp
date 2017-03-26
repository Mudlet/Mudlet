/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2014 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "dlgAboutDialog.h"


#include "pre_guard.h"
#include <QPainter>
#include <QTextLayout>
#include <QStringBuilder>
#include "post_guard.h"


dlgAboutDialog::dlgAboutDialog(QWidget * parent) : QDialog(parent)
{
    setupUi(this);

    // Copied from main():

    QImage splashImage(":/Mudlet_splashscreen_main.png");

    { // Brace code using painter to ensure it is freed at right time...
        QPainter painter( &splashImage );

        unsigned fontSize = 16;
        QString sourceVersionText = QString( "Version: " APP_VERSION APP_BUILD );

        bool isWithinSpace = false;
        while( ! isWithinSpace )
        {
            QFont font( "DejaVu Serif", fontSize, QFont::Bold|QFont::Serif|QFont::PreferMatch|QFont::PreferAntialias );
            QTextLayout versionTextLayout( sourceVersionText, font, painter.device() );
            versionTextLayout.beginLayout();
            // Start work in this text item
            QTextLine versionTextline = versionTextLayout.createLine();
            // First draw (one line from) the text we have put in on the layout to
            // see how wide it is..., assuming accutally that it will only take one
            // line of text
            versionTextline.setLineWidth( 280 );
            //Splashscreen bitmap is (now) 320x360 - hopefully entire line will all fit into 280
            versionTextline.setPosition( QPointF( 0, 0 ) );
            // Only pretend, so we can see how much space it will take
            QTextLine dummy = versionTextLayout.createLine();
            if( ! dummy.isValid() )
            { // No second line so have got all text in first so can do it
                isWithinSpace = true;
                qreal versionTextWidth = versionTextline.naturalTextWidth();
                // This is the ACTUAL width of the created text
                versionTextline.setPosition( QPointF( (320 - versionTextWidth) / 2.0 , 270 ) );
                // And now we can place it centred horizontally
                versionTextLayout.endLayout();
                // end the layout process and paint it out
                painter.setPen( QColor( 176, 64, 0, 255 ) ); // #b04000
                versionTextLayout.draw( &painter, QPointF( 0, 0 ) );
            }
            else
            { // Too big - text has spilled over onto a second line - so try again
                fontSize--;
                versionTextLayout.clearLayout();
                versionTextLayout.endLayout();
            }
        }

        // Repeat for other text, but we know it will fit at given size
        QString sourceCopyrightText = QChar( 169 ) % QString( " Heiko K" ) % QChar( 246 ) % QString( "hn 2008-" ) % QString(__DATE__).mid(7);
        QFont font( "DejaVu Serif", 16, QFont::Bold|QFont::Serif|QFont::PreferMatch|QFont::PreferAntialias );
        QTextLayout copyrightTextLayout( sourceCopyrightText, font, painter.device() );
        copyrightTextLayout.beginLayout();
        QTextLine copyrightTextline = copyrightTextLayout.createLine();
        copyrightTextline.setLineWidth( 280 );
        copyrightTextline.setPosition( QPointF( 1, 1 ) );
        qreal copyrightTextWidth = copyrightTextline.naturalTextWidth();
        copyrightTextline.setPosition( QPointF( (320 - copyrightTextWidth) / 2.0 , 340 ) );
        copyrightTextLayout.endLayout();
        painter.setPen( QColor( 112, 16, 0, 255 ) ); // #701000
        copyrightTextLayout.draw( &painter, QPointF( 0, 0 ) );
    }

    mudletTitleLabel->setPixmap( QPixmap::fromImage( splashImage ) );
}
