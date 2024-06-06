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

#ifndef Patternist_ApplyTemplate_H
#define Patternist_ApplyTemplate_H

#include <private/qtemplatemode_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short When combined with other components, implements
     * @c xsl:apply-templates.
     *
     * Note that ApplyTemplate isn't named ApplyTemplates. The reason for this
     * is that ApplyTemplate doesn't do the iteration part. An @c
     * <xsl:apply-templates/> instruction is rewritten into:
     *
     * @code
     * child::node/() map apply-template()
     * @endcode
     *
     * Hence, this expression requires a focus, although it can consist of
     * atomic values.
     *
     * @since 4.5
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_expressions
     */
    class ApplyTemplate : public TemplateInvoker
    {
    public:
        typedef QExplicitlySharedDataPointer<ApplyTemplate> Ptr;

        /**
         * @short @p mode may be @c null. If it is, ApplyTemplate interprets
         * that as that it should use the #current mode.
         *
         * @see StaticContext::currentTemplateMode()
         */
        ApplyTemplate(const TemplateMode::Ptr &mode,
                      const WithParam::Hash &withParams,
                      const TemplateMode::Ptr &defaultMode);

        virtual Item::Iterator::Ptr evaluateSequence(const DynamicContext::Ptr &context) const;

        virtual SequenceType::Ptr staticType() const;
        virtual ExpressionVisitorResult::Ptr accept(const ExpressionVisitor::Ptr &visitor) const;
        virtual Properties properties() const;

        /**
         * The only reason this function exists, is in order to convert
         * QXmlNodeModelIndex to QPatternist::Item. So, this is a huge
         * performance setback. It applies for one of the builtin templates.
         */
        inline Item mapToItem(const QXmlNodeModelIndex &node,
                              const DynamicContext::Ptr &context) const;
        inline Item::Iterator::Ptr mapToSequence(const Item &item,
                                                 const DynamicContext::Ptr &context) const;

        inline TemplateMode::Ptr mode() const;

        virtual bool configureRecursion(const CallTargetDescription::Ptr &sign);
        virtual Expression::Ptr body() const;
        virtual CallTargetDescription::Ptr callTargetDescription() const;

        Expression::Ptr compress(const StaticContext::Ptr &context);

    private:
        typedef QExplicitlySharedDataPointer<const ApplyTemplate> ConstPtr;

        Template::Ptr findTemplate(const DynamicContext::Ptr &context,
                                   const TemplateMode::Ptr &templateMode) const;
        /**
         * @note You typically want to use effectiveMode().
         */
        const TemplateMode::Ptr m_mode;

        TemplateMode::Ptr m_defaultMode;

        inline TemplateMode::Ptr effectiveMode(const DynamicContext::Ptr &context) const;
    };

    TemplateMode::Ptr ApplyTemplate::mode() const
    {
        return m_mode;
    }
}

QT_END_NAMESPACE

#endif
