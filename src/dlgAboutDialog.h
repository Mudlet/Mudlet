#ifndef MUDLET_DLGABOUTDIALOG_H
#define MUDLET_DLGABOUTDIALOG_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "pre_guard.h"
#include "ui_about_dialog.h"
#include <memory>
#include "post_guard.h"

struct aboutMaker {
  bool big;
  QString name;
  QString discord;
  QString github;
  QString email;
  QString description;
};

class dlgAboutDialog : public QDialog, public Ui::about_dialog
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgAboutDialog)
    dlgAboutDialog(QWidget* parent = nullptr);

private:
    std::unique_ptr<QTextDocument> supportersDocument;
    void setAboutTab(const QString& htmlHead) const;
    void setLicenseTab(const QString& htmlHead) const;
    void setThirdPartyTab(const QString& htmlHead) const;
    void setSupportersTab(const QString &htmlHead);
    QString createMakerHTML(const aboutMaker&) const;
};

#endif // MUDLET_DLGABOUTDIALOG_H
