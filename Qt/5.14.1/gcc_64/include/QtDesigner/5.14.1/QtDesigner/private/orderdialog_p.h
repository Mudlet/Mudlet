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
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef ORDERDIALOG_P_H
#define ORDERDIALOG_P_H

#include "shared_global_p.h"

#include <QtWidgets/qdialog.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;

namespace qdesigner_internal {

namespace Ui {
    class OrderDialog;
}

class QDESIGNER_SHARED_EXPORT OrderDialog: public QDialog
{
    Q_OBJECT
public:
    OrderDialog(QWidget *parent);
    ~OrderDialog() override;

    static QWidgetList pagesOfContainer(const QDesignerFormEditorInterface *core, QWidget *container);

    void setPageList(const QWidgetList &pages);
    QWidgetList pageList() const;

    void setDescription(const QString &d);

    enum Format {        // Display format
        PageOrderFormat, // Container pages, ranging 0..[n-1]
        TabOrderFormat   // List of widgets,  ranging 1..1
    };

    void setFormat(Format f)  { m_format = f; }
    Format format() const     { return m_format; }

private slots:
    void on_upButton_clicked();
    void on_downButton_clicked();
    void on_pageList_currentRowChanged(int row);
    void slotEnableButtonsAfterDnD();
    void slotReset();

private:
    void buildList();
    void enableButtons(int r);

    typedef QMap<int, QWidget*> OrderMap;
    OrderMap m_orderMap;
    Ui::OrderDialog* m_ui;
    Format m_format;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ORDERDIALOG_P_H
