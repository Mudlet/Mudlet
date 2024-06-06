/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLTABLEMODELCOLUMN_P_H
#define QQMLTABLEMODELCOLUMN_P_H

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

#include <QtCore/QObject>
#include <QtQml/qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/qjsvalue.h>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_PRIVATE_EXPORT QQmlTableModelColumn : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue display READ display WRITE setDisplay NOTIFY displayChanged FINAL)
    Q_PROPERTY(QJSValue setDisplay READ getSetDisplay WRITE setSetDisplay NOTIFY setDisplayChanged)
    Q_PROPERTY(QJSValue decoration READ decoration WRITE setDecoration NOTIFY decorationChanged FINAL)
    Q_PROPERTY(QJSValue setDecoration READ getSetDecoration WRITE setSetDecoration NOTIFY setDecorationChanged FINAL)
    Q_PROPERTY(QJSValue edit READ edit WRITE setEdit NOTIFY editChanged FINAL)
    Q_PROPERTY(QJSValue setEdit READ getSetEdit WRITE setSetEdit NOTIFY setEditChanged FINAL)
    Q_PROPERTY(QJSValue toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged FINAL)
    Q_PROPERTY(QJSValue setToolTip READ getSetToolTip WRITE setSetToolTip NOTIFY setToolTipChanged FINAL)
    Q_PROPERTY(QJSValue statusTip READ statusTip WRITE setStatusTip NOTIFY statusTipChanged FINAL)
    Q_PROPERTY(QJSValue setStatusTip READ getSetStatusTip WRITE setSetStatusTip NOTIFY setStatusTipChanged FINAL)
    Q_PROPERTY(QJSValue whatsThis READ whatsThis WRITE setWhatsThis NOTIFY whatsThisChanged FINAL)
    Q_PROPERTY(QJSValue setWhatsThis READ getSetWhatsThis WRITE setSetWhatsThis NOTIFY setWhatsThisChanged FINAL)

    Q_PROPERTY(QJSValue font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QJSValue setFont READ getSetFont WRITE setSetFont NOTIFY setFontChanged FINAL)
    Q_PROPERTY(QJSValue textAlignment READ textAlignment WRITE setTextAlignment NOTIFY textAlignmentChanged FINAL)
    Q_PROPERTY(QJSValue setTextAlignment READ getSetTextAlignment WRITE setSetTextAlignment NOTIFY setTextAlignmentChanged FINAL)
    Q_PROPERTY(QJSValue background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QJSValue setBackground READ getSetBackground WRITE setSetBackground NOTIFY setBackgroundChanged FINAL)
    Q_PROPERTY(QJSValue foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(QJSValue setForeground READ getSetForeground WRITE setSetForeground NOTIFY setForegroundChanged FINAL)
    Q_PROPERTY(QJSValue checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged FINAL)
    Q_PROPERTY(QJSValue setCheckState READ getSetCheckState WRITE setSetCheckState NOTIFY setCheckStateChanged FINAL)

    Q_PROPERTY(QJSValue accessibleText READ accessibleText WRITE setAccessibleText NOTIFY accessibleTextChanged FINAL)
    Q_PROPERTY(QJSValue setAccessibleText READ getSetAccessibleText WRITE setSetAccessibleText NOTIFY setAccessibleTextChanged FINAL)
    Q_PROPERTY(QJSValue accessibleDescription READ accessibleDescription
        WRITE setAccessibleDescription NOTIFY accessibleDescriptionChanged FINAL)
    Q_PROPERTY(QJSValue setAccessibleDescription READ getSetAccessibleDescription
        WRITE setSetAccessibleDescription NOTIFY setAccessibleDescriptionChanged FINAL)

    Q_PROPERTY(QJSValue sizeHint READ sizeHint WRITE setSizeHint NOTIFY sizeHintChanged FINAL)
    Q_PROPERTY(QJSValue setSizeHint READ getSetSizeHint WRITE setSetSizeHint NOTIFY setSizeHintChanged FINAL)

public:
    QQmlTableModelColumn(QObject *parent = nullptr);
    ~QQmlTableModelColumn() override;

    QJSValue display() const;
    void setDisplay(const QJSValue &stringOrFunction);
    QJSValue getSetDisplay() const;
    void setSetDisplay(const QJSValue &function);

    QJSValue decoration() const;
    void setDecoration(const QJSValue &stringOrFunction);
    QJSValue getSetDecoration() const;
    void setSetDecoration(const QJSValue &function);

    QJSValue edit() const;
    void setEdit(const QJSValue &stringOrFunction);
    QJSValue getSetEdit() const;
    void setSetEdit(const QJSValue &function);

    QJSValue toolTip() const;
    void setToolTip(const QJSValue &stringOrFunction);
    QJSValue getSetToolTip() const;
    void setSetToolTip(const QJSValue &function);

    QJSValue statusTip() const;
    void setStatusTip(const QJSValue &stringOrFunction);
    QJSValue getSetStatusTip() const;
    void setSetStatusTip(const QJSValue &function);

    QJSValue whatsThis() const;
    void setWhatsThis(const QJSValue &stringOrFunction);
    QJSValue getSetWhatsThis() const;
    void setSetWhatsThis(const QJSValue &function);

    QJSValue font() const;
    void setFont(const QJSValue &stringOrFunction);
    QJSValue getSetFont() const;
    void setSetFont(const QJSValue &function);

    QJSValue textAlignment() const;
    void setTextAlignment(const QJSValue &stringOrFunction);
    QJSValue getSetTextAlignment() const;
    void setSetTextAlignment(const QJSValue &function);

    QJSValue background() const;
    void setBackground(const QJSValue &stringOrFunction);
    QJSValue getSetBackground() const;
    void setSetBackground(const QJSValue &function);

    QJSValue foreground() const;
    void setForeground(const QJSValue &stringOrFunction);
    QJSValue getSetForeground() const;
    void setSetForeground(const QJSValue &function);

    QJSValue checkState() const;
    void setCheckState(const QJSValue &stringOrFunction);
    QJSValue getSetCheckState() const;
    void setSetCheckState(const QJSValue &function);

    QJSValue accessibleText() const;
    void setAccessibleText(const QJSValue &stringOrFunction);
    QJSValue getSetAccessibleText() const;
    void setSetAccessibleText(const QJSValue &function);

    QJSValue accessibleDescription() const;
    void setAccessibleDescription(const QJSValue &stringOrFunction);
    QJSValue getSetAccessibleDescription() const;
    void setSetAccessibleDescription(const QJSValue &function);

    QJSValue sizeHint() const;
    void setSizeHint(const QJSValue &stringOrFunction);
    QJSValue getSetSizeHint() const;
    void setSetSizeHint(const QJSValue &function);

    QJSValue getterAtRole(const QString &roleName);
    QJSValue setterAtRole(const QString &roleName);

    const QHash<QString, QJSValue> getters() const;

    static const QHash<int, QString> supportedRoleNames();

Q_SIGNALS:
    void indexChanged();
    void displayChanged();
    void setDisplayChanged();
    void decorationChanged();
    void setDecorationChanged();
    void editChanged();
    void setEditChanged();
    void toolTipChanged();
    void setToolTipChanged();
    void statusTipChanged();
    void setStatusTipChanged();
    void whatsThisChanged();
    void setWhatsThisChanged();

    void fontChanged();
    void setFontChanged();
    void textAlignmentChanged();
    void setTextAlignmentChanged();
    void backgroundChanged();
    void setBackgroundChanged();
    void foregroundChanged();
    void setForegroundChanged();
    void checkStateChanged();
    void setCheckStateChanged();

    void accessibleTextChanged();
    void setAccessibleTextChanged();
    void accessibleDescriptionChanged();
    void setAccessibleDescriptionChanged();
    void sizeHintChanged();
    void setSizeHintChanged();

private:
    int mIndex = -1;

    // We store these in hashes because QQuickTableModel needs string-based lookup in certain situations.
    QHash<QString, QJSValue> mGetters;
    QHash<QString, QJSValue> mSetters;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlTableModelColumn)

#endif // QQMLTABLEMODELCOLUMN_P_H
