/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ACTIONPROVIDER_H
#define ACTIONPROVIDER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtDesigner/extension.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtWidgets/qapplication.h>

QT_BEGIN_NAMESPACE

class QAction;

class QDesignerActionProviderExtension
{
public:
    virtual ~QDesignerActionProviderExtension() = default;

    virtual QRect actionGeometry(QAction *action) const = 0;
    virtual QAction *actionAt(const QPoint &pos) const = 0;

    virtual void adjustIndicator(const QPoint &pos) = 0;
};

// Find action at the given position for a widget that has actionGeometry() (QToolBar,
// QMenuBar, QMenu). They usually have actionAt(), but that fails since Designer usually sets
// WA_TransparentForMouseEvents on the widgets.
template <class Widget>
    int actionIndexAt(const Widget *w, const QPoint &pos, Qt::Orientation orientation)
{
    const QList<QAction*> actions = w->actions();
    const int actionCount = actions.count();
    if (actionCount == 0)
        return -1;
    // actionGeometry() can be wrong sometimes; it returns a geometry that
    // stretches to the end of the toolbar/menu bar. So, check from the beginning
    // in the case of a horizontal right-to-left orientation.
    const bool checkTopRight = orientation == Qt::Horizontal && w->layoutDirection() == Qt::RightToLeft;
    const QPoint topRight = QPoint(w->rect().width(), 0);
    for (int index = 0; index < actionCount; ++index) {
        QRect g = w->actionGeometry(actions.at(index));
        if (checkTopRight)
            g.setTopRight(topRight);
        else
            g.setTopLeft(QPoint(0, 0));

        if (g.contains(pos))
            return index;
    }
    return -1;
}

Q_DECLARE_EXTENSION_INTERFACE(QDesignerActionProviderExtension, "org.qt-project.Qt.Designer.ActionProvider")

QT_END_NAMESPACE

#endif // ACTIONPROVIDER_H
