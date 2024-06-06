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

#ifndef QDESIGNER_WIDGETBOX_H
#define QDESIGNER_WIDGETBOX_H

#include "shared_global_p.h"
#include <QtDesigner/abstractwidgetbox.h>

QT_BEGIN_NAMESPACE

class DomUI;

namespace qdesigner_internal {

// A widget box with a load mode that allows for updating custom widgets.

class QDESIGNER_SHARED_EXPORT QDesignerWidgetBox : public QDesignerWidgetBoxInterface
{
    Q_OBJECT
public:
    enum LoadMode { LoadMerge, LoadReplace, LoadCustomWidgetsOnly };

    explicit QDesignerWidgetBox(QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr);

    LoadMode loadMode() const;
    void setLoadMode(LoadMode lm);

    virtual bool loadContents(const QString &contents) = 0;

    // Convenience to access the widget box icon of a widget. Empty category
    // matches all
    virtual QIcon iconForWidget(const QString &className,
                                const QString &category = QString()) const = 0;

    // Convenience to find a widget by class name. Empty category matches all
    static bool findWidget(const QDesignerWidgetBoxInterface *wbox,
                           const QString &className,
                           const QString &category /* = QString()  */,
                           Widget *widgetData);
    // Convenience functions to create a DomWidget from widget box xml.
    static DomUI *xmlToUi(const QString &name, const QString &xml, bool insertFakeTopLevel, QString *errorMessage);
    static DomUI *xmlToUi(const QString &name, const QString &xml, bool insertFakeTopLevel);

private:
    LoadMode m_loadMode = LoadMerge;
};
}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_WIDGETBOX_H
