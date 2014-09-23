/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "colorswizardpage.h"
#include <QStyledItemDelegate>
#include <QComboBox>

enum Columns
{
    Message,
    Color
};

// SVG color keyword names provided by the World Wide Web Consortium
static const QStringList COLORS = QStringList()
    << "aliceblue" << "antiquewhite" << "aqua" << "aquamarine" << "azure" << "beige" << "bisque"
    << "black" << "blanchedalmond" << "blue" << "blueviolet" << "brown" << "burlywood" << "cadetblue"
    << "chartreuse" << "chocolate" << "coral" << "cornflowerblue" << "cornsilk" << "crimson" << "cyan"
    << "darkblue" << "darkcyan" << "darkgoldenrod" << "darkgray" << "darkgreen" << "darkgrey"
    << "darkkhaki" << "darkmagenta" << "darkolivegreen" << "darkorange" << "darkorchid" << "darkred"
    << "darksalmon" << "darkseagreen" << "darkslateblue" << "darkslategray" << "darkslategrey"
    << "darkturquoise" << "darkviolet" << "deeppink" << "deepskyblue" << "dimgray" << "dimgrey"
    << "dodgerblue" << "firebrick" << "floralwhite" << "forestgreen" << "fuchsia" << "gainsboro"
    << "ghostwhite" << "gold" << "goldenrod" << "gray" << "grey" << "green" << "greenyellow"
    << "honeydew" << "hotpink" << "indianred" << "indigo" << "ivory" << "khaki" << "lavender"
    << "lavenderblush" << "lawngreen" << "lemonchiffon" << "lightblue" << "lightcoral" << "lightcyan"
    << "lightgoldenrodyellow" << "lightgray" << "lightgreen" << "lightgrey" << "lightpink"
    << "lightsalmon" << "lightseagreen" << "lightskyblue" << "lightslategray" << "lightslategrey"
    << "lightsteelblue" << "lightyellow" << "lime" << "limegreen" << "linen" << "magenta"
    << "maroon" << "mediumaquamarine" << "mediumblue" << "mediumorchid" << "mediumpurple"
    << "mediumseagreen" << "mediumslateblue" << "mediumspringgreen" << "mediumturquoise"
    << "mediumvioletred" << "midnightblue" << "mintcream" << "mistyrose" << "moccasin"
    << "navajowhite" << "navy" << "oldlace" << "olive" << "olivedrab" << "orange" << "orangered"
    << "orchid" << "palegoldenrod" << "palegreen" << "paleturquoise" << "palevioletred"
    << "papayawhip" << "peachpuff" << "peru" << "pink" << "plum" << "powderblue" << "purple" << "red"
    << "rosybrown" << "royalblue" << "saddlebrown" << "salmon" << "sandybrown" << "seagreen"
    << "seashell" << "sienna" << "silver" << "skyblue" << "slateblue" << "slategray" << "slategrey"
    << "snow" << "springgreen" << "steelblue" << "tan" << "teal" << "thistle" << "tomato"
    << "turquoise" << "violet" << "wheat" << "white" << "whitesmoke" << "yellow" << "yellowgreen";

class ColorItemDelegate : public QStyledItemDelegate
{
public:
    ColorItemDelegate(QObject* parent = 0) : QStyledItemDelegate(parent)
    {
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        Q_UNUSED(option);
        if (index.column() == Color)
        {
            QComboBox* comboBox = new QComboBox(parent);
            comboBox->addItems(COLORS);
            int i = 0;
            foreach (const QString& color, COLORS)
                comboBox->setItemData(i++, QColor(color), Qt::DecorationRole);
            return comboBox;
        }
        return 0;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        if (index.column() == Color)
        {
            QComboBox* comboBox = static_cast<QComboBox*>(editor);
            int i = comboBox->findText(index.data().toString());
            comboBox->setCurrentIndex(i);
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        if (index.column() == Color)
        {
            QComboBox* comboBox = static_cast<QComboBox*>(editor);
            model->setData(index, comboBox->currentText());
            model->setData(index, QColor(comboBox->currentText()), Qt::DecorationRole);
        }
    }
};

ColorsWizardPage::ColorsWizardPage(QWidget* parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/oxygen/64x64/actions/color_line.png"));
    ui.treeWidget->setItemDelegate(new ColorItemDelegate(ui.treeWidget));
    ui.treeWidget->header()->setResizeMode(Message, QHeaderView::ResizeToContents);
}

QHash<int, QString> ColorsWizardPage::colors() const
{
    QHash<int, QString> colors;
    for (int i = Settings::Background; i <= Settings::Link; ++i)
        colors[i] = ui.treeWidget->topLevelItem(i)->data(Color, Qt::DisplayRole).toString();
    return colors;
}

void ColorsWizardPage::setColors(const QHash<int, QString>& colors)
{
    QHashIterator<int, QString> it(colors);
    while (it.hasNext())
    {
        it.next();
        ui.treeWidget->topLevelItem(it.key())->setData(Color, Qt::DisplayRole, it.value());
        ui.treeWidget->topLevelItem(it.key())->setData(Color, Qt::DecorationRole, QColor(it.value()));
    }
}
