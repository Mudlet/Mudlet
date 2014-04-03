/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

dlgAboutDialog::dlgAboutDialog(QWidget * parent) : QDialog(parent)
{
    setupUi(this);
    mudletTitleLabel->setTextFormat(Qt::RichText);
    mudletTitleLabel->setText( QString("<p align=\"center\" style=\" margin-top:6px; margin-bottom:6px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; \"><span style=\" font-family:\'DejaVu Sans\'; font-size:14pt; font-weight:600; color:#c04000;\">Version: %1%2</span><br><br><br><span style=\" font-family:\'DejaVu Sans\'; font-size:14pt; font-weight:600; color:#801000; \">&#169; Heiko K&#246;hn 2008-%3</span></p>")
                                       .arg(APP_VERSION)
                                       .arg(APP_BUILD)
                                       .arg(QString(__DATE__).mid(7)) );
}

