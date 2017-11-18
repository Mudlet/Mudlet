#ifndef MUDLET_TLABEL_H
#define MUDLET_TLABEL_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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

#include "TEvent.h"

#include "post_guard.h"
#include "pre_guard.h"
#include <QLabel>
#include <QPointer>
#include <QString>

class Host;

class QMouseEvent;

class TLabel : public QLabel {
  Q_OBJECT

public:
  Q_DISABLE_COPY(TLabel)
  TLabel(QWidget *pW = 0);
  void setClick(Host *pHost, const QString &func, const TEvent &args);
  void setRelease(Host *pHost, const QString &func, const TEvent &args);
  void setEnter(Host *pHost, const QString &func, const TEvent &args);
  void setLeave(Host *pHost, const QString &func, const TEvent &args);
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void leaveEvent(QEvent *) override;
  void enterEvent(QEvent *) override;

  bool forwardEventToMapper(QEvent *);

  QPointer<Host> mpHost;
  QString mClick;
  QString mRelease;
  QString mEnter;
  QString mLeave;
  TEvent mClickParams;
  TEvent mReleaseParams;
  TEvent mLeaveParams;
  TEvent mEnterParams;
  bool mouseInside;
};

#endif // MUDLET_TLABEL_H
