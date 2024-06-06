/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QURL_P_H
#define QURL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qurl*.cpp This header file may change from version to version without
// notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "qurl.h"

QT_BEGIN_NAMESPACE

// in qurlrecode.cpp
extern Q_AUTOTEST_EXPORT int qt_urlRecode(QString &appendTo, const QChar *begin, const QChar *end,
                                          QUrl::ComponentFormattingOptions encoding, const ushort *tableModifications = nullptr);

// in qurlidna.cpp
enum AceLeadingDot { AllowLeadingDot, ForbidLeadingDot };
enum AceOperation { ToAceOnly, NormalizeAce };
extern QString qt_ACE_do(const QString &domain, AceOperation op, AceLeadingDot dot);
extern Q_AUTOTEST_EXPORT bool qt_nameprep(QString *source, int from);
extern Q_AUTOTEST_EXPORT bool qt_check_std3rules(const QChar *uc, int len);
extern Q_AUTOTEST_EXPORT void qt_punycodeEncoder(const QChar *s, int ucLength, QString *output);
extern Q_AUTOTEST_EXPORT QString qt_punycodeDecoder(const QString &pc);
extern Q_AUTOTEST_EXPORT QString qt_urlRecodeByteArray(const QByteArray &ba);

QT_END_NAMESPACE

#endif // QURL_P_H
