/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtXmlPatterns module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Patternist_AtomicMathematician_H
#define Patternist_AtomicMathematician_H

#include <QFlags>

#include <private/qdynamiccontext_p.h>
#include <private/qitem_p.h>
#include <private/qatomictypedispatch_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short Base class for classes that performs arithmetic operations between atomic values.
     *
     * @ingroup Patternist_xdm
     * @author Frans Englich <frans.englich@nokia.com>
     */
    class Q_AUTOTEST_EXPORT AtomicMathematician : public AtomicTypeVisitorResult
    {
    public:
        virtual ~AtomicMathematician();

        typedef QExplicitlySharedDataPointer<AtomicMathematician> Ptr;

        enum Operator
        {
            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-divide">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.4 op:numeric-divide</a>
             */
            Div         = 1,

            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-integer-divide">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.5 op:numeric-integer-divide</a>
             */
            IDiv        = 2,

            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-subtract">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.2 op:numeric-subtract</a>
             */
            Substract   = 4,

            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-mod">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.6 op:numeric-mod</a>
             */
            Mod         = 8,

            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-multiply">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.3 op:numeric-multiply</a>
             */
            Multiply    = 16,

            /**
             * @see <a href="http://www.w3.org/TR/xpath-functions/#func-numeric-add">XQuery 1.0
             * and XPath 2.0 Functions and Operators, 6.2.1 op:numeric-add</a>
             */
            Add         = 32
        };

        typedef QFlags<Operator> Operators;

        virtual Item calculate(const Item &operand1,
                                    const Operator op,
                                    const Item &operand2,
                                    const QExplicitlySharedDataPointer<DynamicContext> &context) const = 0;

        static QString displayName(const AtomicMathematician::Operator op);

    };
    Q_DECLARE_OPERATORS_FOR_FLAGS(AtomicMathematician::Operators)
}

QT_END_NAMESPACE

#endif
