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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef LAYOUTINFO_H
#define LAYOUTINFO_H

#include "shared_global_p.h"

QT_BEGIN_NAMESPACE

class QWidget;
class QLayout;
class QLayoutItem;
class QDesignerFormEditorInterface;
class QFormLayout;
class QRect;
class QString;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT LayoutInfo
{
public:
    enum Type
    {
        NoLayout,
        HSplitter,
        VSplitter,
        HBox,
        VBox,
        Grid,
        Form,
        UnknownLayout // QDockWindow inside QMainWindow is inside QMainWindowLayout - it doesn't mean there is no layout
    };

    static void deleteLayout(const QDesignerFormEditorInterface *core, QWidget *widget);

    // Examines the immediate layout of the widget.
    static Type layoutType(const QDesignerFormEditorInterface *core, const QWidget *w);
    // Examines the managed layout of the widget
    static Type managedLayoutType(const QDesignerFormEditorInterface *core, const QWidget *w, QLayout **layout = nullptr);
    static Type layoutType(const QDesignerFormEditorInterface *core, const QLayout *layout);
    static Type layoutType(const QString &typeName);
    static QString layoutName(Type t);

    static QWidget *layoutParent(const QDesignerFormEditorInterface *core, QLayout *layout);

    static Type laidoutWidgetType(const QDesignerFormEditorInterface *core, QWidget *widget, bool *isManaged = nullptr, QLayout **layout = nullptr);
    static bool inline isWidgetLaidout(const QDesignerFormEditorInterface *core, QWidget *widget) { return laidoutWidgetType(core, widget) != NoLayout; }

    static QLayout *managedLayout(const QDesignerFormEditorInterface *core, const QWidget *widget);
    static QLayout *managedLayout(const QDesignerFormEditorInterface *core, QLayout *layout);
    static QLayout *internalLayout(const QWidget *widget);

    // Is it a a dummy grid placeholder created by Designer?
    static bool isEmptyItem(QLayoutItem *item);
};

QDESIGNER_SHARED_EXPORT void getFormLayoutItemPosition(const QFormLayout *formLayout, int index, int *rowPtr, int *columnPtr = nullptr, int *rowspanPtr = nullptr, int *colspanPtr = nullptr);
QDESIGNER_SHARED_EXPORT void formLayoutAddWidget(QFormLayout *formLayout, QWidget *w, const QRect &r, bool insert);
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // LAYOUTINFO_H
