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

#ifndef Patternist_CastAs_H
#define Patternist_CastAs_H

#include <private/qsinglecontainer_p.h>
#include <private/qcastingplatform_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short Implements XPath 2.0's <tt>cast as</tt> expression.
     *
     * Implements the casting expression, such as <tt>'3' cast as xs:integer</tt>. This class also
     * implements constructor functions, which are created in the ConstructorFunctionsFactory.
     *
     * CastAs uses CastingPlatform for carrying out the actual casting.
     *
     * @see <a href="http://www.w3.org/TR/xpath-functions/#casting">XQuery 1.0
     * and XPath 2.0 Functions and Operators, 7 Casting</a>
     * @see <a href="http://www.w3.org/TR/xpath20/#id-cast">XML Path Language
     * (XPath) 2.0, 3.10.2 Cast</a>
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_expressions
     */
    class CastAs : public SingleContainer,
                   public CastingPlatform<CastAs, true /* issueError */>
    {
    public:

        /**
         * @todo Wrong/old documentation
         *
         * Creates a cast expression for the type @p name via the schema type
         * factory @p factory. This function is used by parser when creating
         * 'cast to' expressions, and the ConstructorFunctionsFactory, when creating
         * constructor functions.
         *
         * @param targetType the type which the CastAs should cast to
         * @param source the operand to evaluate and then cast from
         */
        CastAs(const Expression::Ptr &source,
               const SequenceType::Ptr &targetType);

        virtual Item evaluateSingleton(const DynamicContext::Ptr &) const;

        virtual SequenceType::List expectedOperandTypes() const;

        /**
         * @returns a SequenceType where the ItemType is this CastAs's
         * target type, as per targetType(), and the Cardinality is inferred from the
         * source operand to reflect whether this CastAs always will evaluate to
         * exactly-one or zero-or-one values.
         */
        virtual SequenceType::Ptr staticType() const;

        /**
         * Overridden in order to check that casting to an abstract type
         * is not attempted.
         */
        virtual Expression::Ptr typeCheck(const StaticContext::Ptr &context,
                                          const SequenceType::Ptr &reqType);

        /**
         * If the target type is the same as the source type, it is rewritten
         * to the operand.
         */
        virtual Expression::Ptr compress(const StaticContext::Ptr &context);
        virtual ExpressionVisitorResult::Ptr accept(const ExpressionVisitor::Ptr &visitor) const;

        inline ItemType::Ptr targetType() const
        {
            return m_targetType->itemType();
        }

        inline SequenceType::Ptr targetSequenceType() const
        {
            return m_targetType;
        }

    private:
        /**
         * Performs casting to @c xs:QName. This case is special, and is always done at compile time.
         */
        Expression::Ptr castToQName(const StaticContext::Ptr &context) const;

        const SequenceType::Ptr m_targetType;
    };
}

QT_END_NAMESPACE

#endif
