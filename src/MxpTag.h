/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef MUDLET_MXPTAG_H
#define MUDLET_MXPTAG_H

#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <functional>

class MxpTagAttribute : public QPair<QString, QString>
{
public:
    typedef std::function<MxpTagAttribute(const MxpTagAttribute&)> Transformation;

    MxpTagAttribute(const QString& name, const QString& value) : QPair(name, value) {}

    explicit MxpTagAttribute(const QString& name) : MxpTagAttribute(name, QString::fromLatin1("")) {}

    MxpTagAttribute() : QPair() {}

    virtual const QString& getName() const { return first; }

    inline const QString& getValue() const { return second; }

    inline bool hasValue() const { return !second.isEmpty(); }

    inline bool isNamed(const QString& name) const { return name.compare(first, Qt::CaseInsensitive) == 0; }
};

class MxpTag;
class MxpStartTag;
class MxpEndTag;
class MxpTextNode;

class MxpNode
{
public:
    enum Type { MXP_NODE_TYPE_TEXT, MXP_NODE_TYPE_START_TAG, MXP_NODE_TYPE_END_TAG };

    explicit MxpNode(MxpNode::Type type) : mType(type) {}

    MxpNode::Type getType() const { return mType; }

    MxpTag* asTag() { return mType != MXP_NODE_TYPE_TEXT ? reinterpret_cast<MxpTag*>(this) : nullptr; }

    MxpStartTag* asStartTag() { return mType == MXP_NODE_TYPE_START_TAG ? reinterpret_cast<MxpStartTag*>(this) : nullptr; }

    MxpEndTag* asEndTag() { return mType == MXP_NODE_TYPE_END_TAG ? reinterpret_cast<MxpEndTag*>(this) : nullptr; }

    MxpTextNode* asText() { return mType == MXP_NODE_TYPE_TEXT ? reinterpret_cast<MxpTextNode*>(this)  : nullptr; }

    virtual QString toString() const = 0;

    bool isTag() { return mType != MXP_NODE_TYPE_TEXT; }

    bool isEndTag() { return mType == MXP_NODE_TYPE_END_TAG; }

    bool isStartTag() { return mType == MXP_NODE_TYPE_START_TAG; }

protected:
    MxpNode::Type mType;
};

class MxpTextNode : public MxpNode
{
    QString mContent;

public:
    explicit MxpTextNode(const QString& content) : MxpNode(MXP_NODE_TYPE_TEXT), mContent(QString(content)) {}

    inline const QString& getContent() const { return mContent; }

    virtual QString toString() const { return mContent; }
};

class MxpTag : public MxpNode
{
    friend class TMxpTagParser;

protected:
    QString name;

    explicit MxpTag(MxpNode::Type type, const QString& name) : MxpNode(type), name(name) {}

public:
    inline const QString& getName() const { return name; }

    inline bool isStartTag() const { return mType == MXP_NODE_TYPE_START_TAG; }

    inline bool isEndTag() const { return mType == MXP_NODE_TYPE_END_TAG; }

    bool isNamed(const QString& tagName) const;
};

class MxpEndTag : public MxpTag
{
public:
    explicit MxpEndTag(const QString& name) : MxpTag(MXP_NODE_TYPE_END_TAG, name) {}
    QString toString() const override;
};

class MxpStartTag : public MxpTag
{
    QMap<QString, MxpTagAttribute> mAttrsMap;
    QStringList mAttrsNames;
    bool mIsEmpty;

public:
    explicit MxpStartTag(const QString& name) : MxpStartTag(name, QList<MxpTagAttribute>(), false) {}

    MxpStartTag(const QString& name, const QList<MxpTagAttribute>& attributes, bool isEmpty) : MxpTag(MXP_NODE_TYPE_START_TAG, QString(name)), mIsEmpty(isEmpty)
    {
        for (const auto& attr : attributes) {
            mAttrsNames.append(attr.getName());
            mAttrsMap[attr.first.toUpper()] = attr;
        }
    }

    MxpStartTag transform(const MxpTagAttribute::Transformation& transformation) const;

    inline const QStringList& getAttributesNames() const { return mAttrsNames; }

    inline int getAttributesCount() const { return mAttrsNames.size(); }

    bool hasAttribute(const QString& attrName) const;

    const MxpTagAttribute& getAttribute(int attrIndex) const;

    const MxpTagAttribute& getAttribute(const QString& attrName) const;

    const QString& getAttributeValue(int attrIndex) const;
    const QString& getAttributeValue(const QString& attrName) const;
    const QString& getAttributeByNameOrIndex(const QString& attrName, int attrIndex, const QString& defaultValue = QString()) const;
    bool isAttributeAt(const char* attrName, int attrIndex);
    inline bool isEmpty() const { return mIsEmpty; }

    QString toString() const override;
    const QString& getAttrName(int attrIndex) const;
};

#endif //MUDLET_MXPTAG_H
