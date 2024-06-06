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

#ifndef Patternist_UnlimitedContainer_H
#define Patternist_UnlimitedContainer_H

#include <QList>
#include <private/qexpression_p.h>
#include <private/qgenericsequencetype_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short Base class for expressions that has any amount of operands.
     *
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_expressions
     */
    class UnlimitedContainer : public Expression
    {
    public:
        /**
         * Creates an UnlimitedContainer containing the operands @p operands. @p operands
         * may be empty.
         */
        UnlimitedContainer(const Expression::List &operands = Expression::List());

        virtual void setOperands(const Expression::List &list);

        virtual Expression::List operands() const;

        /**
         * @note This function cannot be called before setOperands is called.
         */
        virtual bool compressOperands(const StaticContext::Ptr &);

        /**
         * Tells how operandsUnionType() should compute the cardinality of
         * its children.
         *
         * This type is public because of a bug in the HP-UX aCC compiler.
         */
        enum CardinalityComputation
        {
            ProductOfCardinality,
            UnionOfCardinality
        };

    protected:
        /**
         * Computes and returns the union type of all the Expression instances
         * in this Expression's operands.
         *
         * This implementation is placed inside because CardinalityComputation
         * can't be referenced from the outside(in conforming compilers).
         */
        template<CardinalityComputation suppliedCard>
        inline
        SequenceType::Ptr operandsUnionType() const
        {
            Q_ASSERT(suppliedCard ==  ProductOfCardinality || suppliedCard == UnionOfCardinality);
            const Expression::List::const_iterator end(m_operands.constEnd());
            Expression::List::const_iterator it(m_operands.constBegin());

            /* Load the first one, and jump over it in the loop. */
            SequenceType::Ptr t(m_operands.first()->staticType());
            ItemType::Ptr type(t->itemType());
            Cardinality card(t->cardinality());
            ++it;

            for(; it != end; ++it)
            {
                t = (*it)->staticType();
                type |= t->itemType();

                /* Since this function is a template function, it doesn't
                 * hurt performance that this test is inside the loop. */
                if(suppliedCard == ProductOfCardinality)
                    card += t->cardinality();
                else
                    card |= t->cardinality();
            }

            return makeGenericSequenceType(type, card);
        }

        Expression::List m_operands;
    };


}

QT_END_NAMESPACE

#endif
