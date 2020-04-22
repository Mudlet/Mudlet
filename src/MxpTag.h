#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
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

#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>
#include <functional>

#include <QDebug>

class MxpTagAttribute : public QPair<QString, QString>
{
    friend class MxpTagLineParser;

public:
    typedef std::function<MxpTagAttribute(const MxpTagAttribute&)> Transformation;

    MxpTagAttribute(const QString& name, const QString& value) : QPair(name, value) {}

    explicit MxpTagAttribute(const QString& name) : MxpTagAttribute(name, QString::fromLatin1("")) {}

    MxpTagAttribute() : QPair() {}

    MxpTagAttribute transform(Transformation transformation) const { return transformation(*this); }

    virtual const QString& getName() const { return first; }

    inline const QString& getValue() const { return second; }

    inline bool hasValue() const { return !second.isEmpty(); }

    inline bool isNamed(const QString& name) const { return name.compare(first, Qt::CaseInsensitive) == 0; }
};

class MxpStartTag;
class MxpEndTag;
class MxpTextNode;

class MxpNode
{
public:
    enum Type { MXP_NODE_TYPE_TEXT, MXP_NODE_TYPE_START_TAG, MXP_NODE_TYPE_END_TAG };

    explicit MxpNode(MxpNode::Type type) : mType(type) {}

    MxpNode::Type getType() const { return mType; }

    MxpStartTag* asStartTag() const { return mType == MXP_NODE_TYPE_START_TAG ? (MxpStartTag*)this : nullptr; }

    MxpEndTag* asEndTag() const { return mType == MXP_NODE_TYPE_END_TAG ? (MxpEndTag*)this : nullptr; }

    MxpTextNode* asText() const { return mType == MXP_NODE_TYPE_TEXT ? (MxpTextNode*)this : nullptr; }

    virtual QString asString() const = 0;

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

    virtual QString asString() const { return mContent; }
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
    QString asString() const override;
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

    inline const QStringList& getAttrsNames() const { return mAttrsNames; }

    inline int getAttrsCount() const { return mAttrsNames.size(); }

    bool hasAttr(const QString& attrName) const;

    const MxpTagAttribute& getAttr(int attrIndex) const;

    const MxpTagAttribute& getAttr(const QString& attrName) const;

    const QString& getAttrValue(int attrIndex) const;
    const QString& getAttrValue(const QString& attrName) const;
    const QString& getAttrByNameOrIndex(const QString& attrName, int attrIndex, const QString& defaultValue = QString::fromLatin1("")) const;
    bool isAttrAt(int pos, const char* attrName);
    inline bool isEmpty() const { return mIsEmpty; }

    QString asString() const override;
    const QString& getAttrName(int attrIndex) const;
};

#endif //MUDLET_MXPTAG_H
