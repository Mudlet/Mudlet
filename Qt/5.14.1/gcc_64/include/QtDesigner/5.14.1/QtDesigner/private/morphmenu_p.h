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

#ifndef MORPH_COMMAND_H
#define MORPH_COMMAND_H

#include "shared_global_p.h"
#include "qdesigner_formwindowcommand_p.h"

QT_BEGIN_NAMESPACE

class QAction;
class QMenu;

namespace qdesigner_internal {

/* Conveniene morph menu that acts on a single widget. */
class QDESIGNER_SHARED_EXPORT MorphMenu : public QObject {
    Q_DISABLE_COPY_MOVE(MorphMenu)
    Q_OBJECT
public:
    using ActionList = QList<QAction *>;

    explicit MorphMenu(QObject *parent = nullptr);

    void populate(QWidget *w, QDesignerFormWindowInterface *fw, ActionList& al);
    void populate(QWidget *w, QDesignerFormWindowInterface *fw, QMenu& m);

private slots:
    void slotMorph(const QString &newClassName);

private:
    bool populateMenu(QWidget *w, QDesignerFormWindowInterface *fw);

    QAction *m_subMenuAction = nullptr;
    QMenu *m_menu = nullptr;
    QWidget *m_widget = nullptr;
    QDesignerFormWindowInterface *m_formWindow = nullptr;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // MORPH_COMMAND_H
