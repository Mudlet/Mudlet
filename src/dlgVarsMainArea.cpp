/***************************************************************************
 *   Copyright (C) 2013 by Chris Mitchell                                  *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgVarsMainArea.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QListWidgetItem>
#include "post_guard.h"

dlgVarsMainArea::dlgVarsMainArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Now needed to setup the (HTML) tool-tips moved into the C++ code here
    // from the form/dialog XML definition which would be subject to QT Designer
    // obfustication...
    slot_guiLanguageChange();
    connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
}

void dlgVarsMainArea::slot_guiLanguageChange()
{
    retranslateUi(this);
    // PLACEMARKER: Redefine GUI Texts

    // Modify the normal QComboBoxes with customised data models that impliment
    // https://stackoverflow.com/a/21376774/4805858 so that individual entries
    // can be "disabled":
    // Key type widget:

    // Magic - part 1 set up a replacement data model:
    QListWidget *contents = new QListWidget(comboBox_variable_key_type);
    contents->hide();
    comboBox_variable_key_type->setModel(contents->model());

    // Now populate the widget - throw away stuff entered from form design
    // before substitute model was attached:
    comboBox_variable_key_type->clear();
    comboBox_variable_key_type->insertItem(0, tr("Auto-Type"), -1); // LUA_TNONE
    comboBox_variable_key_type->insertItem(1, tr("key (string)"), 4);  // LUA_TSTRING
    comboBox_variable_key_type->insertItem(2, tr("index (integer number)"), 3);  // LUA_TNUMBER
    comboBox_variable_key_type->insertItem(3, tr("table (use \"Add Group\" to create)"), 5);  // LUA_TTABLE
    comboBox_variable_key_type->insertItem(4, tr("function (cannot create from GUI)"), 6);  // LUA_TFUNCTION

    // Magic - part 2 use the features of the substitute data model to disable
    // the required entries - they can still be set programmatically for display
    // purposes:
    // Disable table type:
    QListWidgetItem *item = contents->item(3);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    // Disable function type:
    item = contents->item(4);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);


    // Value type widget:
    // Magic - part 1 set up a replacement data model:
    contents = new QListWidget(comboBox_variable_value_type);
    contents->hide();
    comboBox_variable_value_type->setModel(contents->model());

    // Now populate the widget - throw away stuff entered from form design
    // before substitute model was attached:
    comboBox_variable_value_type->clear();
    comboBox_variable_value_type->insertItem(0, tr("Auto-Type"), -1); // LUA_TNONE
    comboBox_variable_value_type->insertItem(1, tr("string"), 4);  // LUA_TSTRING
    comboBox_variable_value_type->insertItem(2, tr("number"), 3);  // LUA_TNUMBER
    comboBox_variable_value_type->insertItem(3, tr("boolean"), 3);  // LUA_TNUMBER
    comboBox_variable_value_type->insertItem(4, tr("table"), 5);  // LUA_TTABLE
    comboBox_variable_value_type->insertItem(5, tr("function"), 6);  // LUA_TFUNCTION

    // Magic - part 2 use the features of the substitute data model:
    // Disable function type:
    item = contents->item(5);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    lineEdit_var_name->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                  .arg(tr("<p>Set the <i>global variable</i> or the <i>table entry</i> name here. The name has to start with a letter, but can contain a mix of letters and numbers.</p>")));
    widget_variable_key_type->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>If this variable is a key of a table, then you can select it to either be an <i>indexed key</i> (unique positive integer with no gaps between it and the first item which has a key of 1) or an <i>associative key</i> (a string or any other number or combination) type.</p>")));
    comboBox_variable_key_type->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                           .arg(tr("<p>Tables can store values either in a list, and/or a hashmap.</p>"
                                                   "<p>In a <b>list</b>, <i>unique indexed keys</i> represent values - so you can have values at <i>1, 2, 3...</i></p>"
                                                   "<p>In a <b>map</b> {a.k.a. an <i>associative array}</i>, <i>unique keys</i> represent values - so you can have values under any identifier you would like (theoretically even a function or other lua entity although this GUI only supports strings).</p>"
                                                   "<p>This, for a newly created table (group) selects whenever you would like your table to be an indexed or an associative one.</p>"
                                                   "<p>In other cases it displays other entities (<i>functions</i>) which cannot be modifed from here.</p>")));
    widget_variable_hide->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                     .arg(tr("<p>Hides this variable (and its children if it is a table) from the Variables view, if the <i>Display All Variables</i> control is off.</p>"
                                             "<p><i>You might want to use this to hide variables that are not related to your scripts, so you can focus only on your own system and not be distrated by some of the Mudlet defaults - or, alternatively, to use this to not mess them by up accident!</i></p>")));
    checkBox_variable_hidden->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>If checked this item (and its children, if applicable) does not show in area to the left unless <i>Show normally hidden variables</i> is checked.</p>")));

    widget_variable_var_type->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                         .arg(tr("<p>Value type the variable data (in the editor box below) should be</p><table border=\"0\" style=\" margin-top:6px; margin-bottom:6px; margin-left:6px; margin-right:6px;\" cellspacing=\"2\" cellpadding=\"2\">"
                                                 "<tr><td><p><i>string</i></p></td><td><p>it is text (can contain numbers)</p></td></tr>"
                                                 "<tr><td><p><i>number</i></p></td><td><p>it is numbers only</p></td></tr>"
                                                 "<tr><td><p><i>boolean</i></p></td><td><p>it is a true or false value only</p></td></tr>"
                                                 "<tr><td><p><i>table</i></p></td><td><p>this is a table (group), and can have other values in it</p></td></tr>"
                                                 "</table></p>")));
}
