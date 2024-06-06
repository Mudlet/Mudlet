/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt SVG module of the Qt Toolkit.
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

#ifndef QSVGHANDLER_P_H
#define QSVGHANDLER_P_H

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

#include "QtCore/qxmlstream.h"
#include "QtCore/qhash.h"
#include "QtCore/qstack.h"
#include <QtCore/QLoggingCategory>
#include "qsvgstyle_p.h"
#include "private/qcssparser_p.h"
#include "qsvggraphics_p.h"
#include "qtsvgglobal_p.h"

QT_BEGIN_NAMESPACE

class QSvgNode;
class QSvgTinyDocument;
class QSvgHandler;
class QColor;
class QSvgStyleSelector;
class QXmlStreamReader;

#ifndef QT_NO_CSSPARSER

struct QSvgCssAttribute
{
    QXmlStreamStringRef name;
    QXmlStreamStringRef value;
};

#endif

class Q_SVG_PRIVATE_EXPORT QSvgHandler
{
public:
    enum LengthType {
        LT_PERCENT,
        LT_PX,
        LT_PC,
        LT_PT,
        LT_MM,
        LT_CM,
        LT_IN,
        LT_OTHER
    };

public:
    QSvgHandler(QIODevice *device);
    QSvgHandler(const QByteArray &data);
    QSvgHandler(QXmlStreamReader *const data);
    ~QSvgHandler();

    QIODevice *device() const;
    QSvgTinyDocument *document() const;

    inline bool ok() const {
        return document() != 0 && !xml->hasError();
    }

    inline QString errorString() const { return xml->errorString(); }
    inline int lineNumber() const { return xml->lineNumber(); }

    void setDefaultCoordinateSystem(LengthType type);
    LengthType defaultCoordinateSystem() const;

    void pushColor(const QColor &color);
    void pushColorCopy();
    void popColor();
    QColor currentColor() const;

#ifndef QT_NO_CSSPARSER
    void setInStyle(bool b);
    bool inStyle() const;

    QSvgStyleSelector *selector() const;
#endif

    void setAnimPeriod(int start, int end);
    int animationDuration() const;

#ifndef QT_NO_CSSPARSER
    void parseCSStoXMLAttrs(const QString &css, QVector<QSvgCssAttribute> *attributes);
#endif

    inline QPen defaultPen() const
    { return m_defaultPen; }

public:
    bool startElement(const QString &localName, const QXmlStreamAttributes &attributes);
    bool endElement(const QStringRef &localName);
    bool characters(const QStringRef &str);
    bool processingInstruction(const QString &target, const QString &data);

private:
    void init();

    QSvgTinyDocument *m_doc;
    QStack<QSvgNode*> m_nodes;

    QList<QSvgNode*>  m_resolveNodes;

    enum CurrentNode
    {
        Unknown,
        Graphics,
        Style
    };
    QStack<CurrentNode> m_skipNodes;

    /*!
        Follows the depths of elements. The top is current xml:space
        value that applies for a given element.
     */
    QStack<QSvgText::WhitespaceMode> m_whitespaceMode;

    QSvgRefCounter<QSvgStyleProperty> m_style;

    LengthType m_defaultCoords;

    QStack<QColor> m_colorStack;
    QStack<int>    m_colorTagCount;

    int m_animEnd;

    QXmlStreamReader *const xml;
#ifndef QT_NO_CSSPARSER
    bool m_inStyle;
    QSvgStyleSelector *m_selector;
    QCss::Parser m_cssParser;
#endif
    void parse();
    void resolveGradients(QSvgNode *node);
    void resolveNodes();

    QPen m_defaultPen;
    /**
     * Whether we own the variable xml, and hence whether
     * we need to delete it.
     */
    const bool m_ownsReader;
};

Q_DECLARE_LOGGING_CATEGORY(lcSvgHandler)

QT_END_NAMESPACE

#endif // QSVGHANDLER_P_H
