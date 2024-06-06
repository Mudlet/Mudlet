/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ABSTRACTFORMBUILDER_H
#define ABSTRACTFORMBUILDER_H

#include "uilib_global.h"

#include <QtCore/qlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>

#include <QtWidgets/qsizepolicy.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE
#if 0
// pragma for syncqt, don't remove.

#pragma qt_class(QAbstractFormBuilder)
#endif

class QAction;
class QButtonGroup;
class QActionGroup;
class QComboBox;
class QIODevice;
class QIcon;
class QLayout;
class QLayoutItem;
class QListWidget;
class QObject;
class QSpacerItem;
class QTreeWidget;
class QTableWidget;
class QVariant;
class QWidget;
class QAbstractButton;
class QAbstractItemView;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class DomAction;
class DomActionGroup;
class DomButtonGroup;
class DomButtonGroups;
class DomActionRef;
class DomBrush;
class DomColorGroup;
class DomConnections;
class DomCustomWidgets;
class DomLayout;
class DomLayoutItem;
class DomProperty;
class DomResources;
class DomSpacer;
class DomString;
class DomTabStops;
class DomUI;
class DomWidget;
class DomResourcePixmap;

class QResourceBuilder;
class QTextBuilder;
class QFormBuilderExtra;

class QDESIGNER_UILIB_EXPORT QAbstractFormBuilder
{
public:
    Q_DISABLE_COPY(QAbstractFormBuilder)

    QAbstractFormBuilder();
    virtual ~QAbstractFormBuilder();

    QDir workingDirectory() const;
    void setWorkingDirectory(const QDir &directory);

    virtual QWidget *load(QIODevice *dev, QWidget *parentWidget = nullptr);
    virtual void save(QIODevice *dev, QWidget *widget);

    QString errorString() const;

protected:
//
// load
//
    virtual void loadExtraInfo(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);

    virtual QWidget *create(DomUI *ui, QWidget *parentWidget);
    virtual QWidget *create(DomWidget *ui_widget, QWidget *parentWidget);
    virtual QLayout *create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget);
    virtual QLayoutItem *create(DomLayoutItem *ui_layoutItem, QLayout *layout, QWidget *parentWidget);

    virtual QAction *create(DomAction *ui_action, QObject *parent);
    virtual QActionGroup *create(DomActionGroup *ui_action_group, QObject *parent);
    virtual void addMenuAction(QAction *action);

    virtual void applyProperties(QObject *o, const QList<DomProperty*> &properties);
    bool applyPropertyInternally(QObject *o, const QString &propertyName, const QVariant &value);

    virtual void applyTabStops(QWidget *widget, DomTabStops *tabStops);

    virtual QWidget *createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name);
    virtual QLayout *createLayout(const QString &layoutName, QObject *parent, const QString &name);
    virtual QAction *createAction(QObject *parent, const QString &name);
    virtual QActionGroup *createActionGroup(QObject *parent, const QString &name);

    virtual void createCustomWidgets(DomCustomWidgets *) {}
    virtual void createConnections(DomConnections *, QWidget *) {}
    virtual void createResources(DomResources*) {}

    virtual bool addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout);
    virtual bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);

//
// save
//
    virtual void saveExtraInfo(QWidget *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);

    virtual void saveDom(DomUI *ui, QWidget *widget);

    virtual DomActionRef *createActionRefDom(QAction *action);

    virtual DomWidget *createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive = true);
    virtual DomLayout *createDom(QLayout *layout, DomLayout *ui_layout, DomWidget *ui_parentWidget);
    virtual DomLayoutItem *createDom(QLayoutItem *item, DomLayout *ui_parentLayout, DomWidget *ui_parentWidget);
    virtual DomSpacer *createDom(QSpacerItem *spacer, DomLayout *ui_parentLayout, DomWidget *ui_parentWidget);

    virtual DomAction *createDom(QAction *action);
    virtual DomActionGroup *createDom(QActionGroup *actionGroup);
    DomButtonGroup *createDom(QButtonGroup *buttonGroup);

    virtual DomConnections *saveConnections();
    virtual DomCustomWidgets *saveCustomWidgets();
    virtual DomTabStops *saveTabStops();
    virtual DomResources *saveResources();
    DomButtonGroups *saveButtonGroups(const QWidget *mainContainer);
    virtual QList<DomProperty*> computeProperties(QObject *obj);
    virtual bool checkProperty(QObject *obj, const QString &prop) const;
    virtual DomProperty *createProperty(QObject *object, const QString &propertyName, const QVariant &value);

    virtual void layoutInfo(DomLayout *layout, QObject *parent, int *margin, int *spacing);

    void loadListWidgetExtraInfo(DomWidget *ui_widget, QListWidget *listWidget, QWidget *parentWidget);
    void loadTreeWidgetExtraInfo(DomWidget *ui_widget, QTreeWidget *treeWidget, QWidget *parentWidget);
    void loadTableWidgetExtraInfo(DomWidget *ui_widget, QTableWidget *tableWidget, QWidget *parentWidget);
    void loadComboBoxExtraInfo(DomWidget *ui_widget, QComboBox *comboBox, QWidget *parentWidget);
    void loadButtonExtraInfo(const DomWidget *ui_widget, QAbstractButton *button, QWidget *parentWidget);
    void loadItemViewExtraInfo(DomWidget *ui_widget, QAbstractItemView *itemView, QWidget *parentWidget);

    void saveListWidgetExtraInfo(QListWidget *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveTreeWidgetExtraInfo(QTreeWidget *treeWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveTableWidgetExtraInfo(QTableWidget *tablWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveComboBoxExtraInfo(QComboBox *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveButtonExtraInfo(const QAbstractButton *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveItemViewExtraInfo(const QAbstractItemView *itemView, DomWidget *ui_widget, DomWidget *ui_parentWidget);

    void setResourceBuilder(QResourceBuilder *builder);
    QResourceBuilder *resourceBuilder() const;
    DomProperty *saveResource(const QVariant &v) const;

    void setTextBuilder(QTextBuilder *builder);
    QTextBuilder *textBuilder() const;
    DomProperty *saveText(const QString &attributeName, const QVariant &v) const;
//
// utils
//

    QVariant toVariant(const QMetaObject *meta, DomProperty *property);
    static QString toString(const DomString *str);

    using DomPropertyHash = QHash<QString, DomProperty*>;
    static DomPropertyHash propertyMap(const QList<DomProperty*> &properties);

    void setupColorGroup(QPalette &palette, QPalette::ColorGroup colorGroup, DomColorGroup *group);
    DomColorGroup *saveColorGroup(const QPalette &palette);
    QBrush setupBrush(DomBrush *brush);
    DomBrush *saveBrush(const QBrush &brush);

    void reset();
    void initialize(const DomUI *ui);

//
//  utils
//

    static QMetaEnum toolBarAreaMetaEnum();

//
//  Icon/pixmap stuff
//
    // A Pair of icon path/qrc path.
    using IconPaths = QPair<QString, QString>;

    IconPaths iconPaths(const QIcon &) const;
    IconPaths pixmapPaths(const QPixmap &) const;
    void setIconProperty(DomProperty &, const IconPaths &) const;
    void setPixmapProperty(DomProperty &, const IconPaths &) const;
    DomProperty* iconToDomProperty(const QIcon &) const;

    static const DomResourcePixmap *domPixmap(const DomProperty* p);
    QIcon domPropertyToIcon(const DomResourcePixmap *);
    QIcon domPropertyToIcon(const DomProperty* p);
    QPixmap domPropertyToPixmap(const DomResourcePixmap* p);
    QPixmap domPropertyToPixmap(const DomProperty* p);

private:
//
//  utils
//
    static Qt::ToolBarArea toolbarAreaFromDOMAttributes(const DomPropertyHash &attributeMap);

    friend QDESIGNER_UILIB_EXPORT DomProperty *variantToDomProperty(QAbstractFormBuilder *abstractFormBuilder, const QMetaObject *meta, const QString &propertyName, const QVariant &value);
    friend QDESIGNER_UILIB_EXPORT QVariant domPropertyToVariant(QAbstractFormBuilder *abstractFormBuilder,const QMetaObject *meta, const DomProperty *property);

protected:
    QScopedPointer<QFormBuilderExtra> d;
};

#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

QT_END_NAMESPACE

#endif // ABSTRACTFORMBUILDER_H
