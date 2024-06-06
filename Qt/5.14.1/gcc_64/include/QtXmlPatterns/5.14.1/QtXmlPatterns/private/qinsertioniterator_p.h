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

#ifndef Patternist_InsertionIterator_H
#define Patternist_InsertionIterator_H

#include <private/qabstractxmlforwarditerator_p.h>
#include <private/qitem_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short Conceptually inserts one QAbstractXmlForwardIterator into another, make two QAbstractXmlForwardIterator instances appear as one.
     *
     * An InsertionIterator represents a sequence that is the merge of two
     * sequences, where one of the iterators is conceptually inserted at a
     * given position. This is done while retaining the characteristic of being
     * pull-based.
     *
     * InsertionIterator contains the logic for the implementation of the <tt>fn:insert-before()</tt>
     * function, whose definition therefore specifies the detailed behaviors of the
     * InsertionIterator.
     *
     * @see <a href="http://www.w3.org/TR/xpath-functions/#func-insert-before">XQuery 1.0
     * and XPath 2.0 Functions and Operators, 15.1.7 fn:insert-before</a>
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_iterators
     */
    class InsertionIterator : public Item::Iterator
    {
    public:

        /**
         * Creates an InsertionIterator whose result is a merge of the
         * iterator @p insertIterator into the iterator @p target at position @p
         * position.
         *
         * @param target the iterator containing the items that the
         * item in @p insertIterator will be inserted into.
         * @param position the insertion position. Must be 1 or larger
         * @param insertIterator the iterator containing the items to insert
         * at position @p position
         */
        InsertionIterator(const Item::Iterator::Ptr &target,
                          const xsInteger position,
                          const Item::Iterator::Ptr &insertIterator);

        virtual Item next();
        virtual Item current() const;
        virtual xsInteger position() const;
        virtual xsInteger count();
        virtual Item::Iterator::Ptr copy() const;

    private:
        const Item::Iterator::Ptr m_target;
        const xsInteger m_insertPos;
        const Item::Iterator::Ptr m_inserts;
        Item m_current;
        xsInteger m_position;
        bool m_isInserting;
    };
}

QT_END_NAMESPACE

#endif
