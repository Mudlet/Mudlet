/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#ifndef dlg_source_editor_area_h
#define dlg_source_editor_area_h

#include "ui_source_editor_area.h"
#include <QWidget>

class THighlighter;

//#include <Qsci/qsciscintilla.h>
//#include <Qsci/qscilexerlua.h>

class dlgSourceEditorArea : public QWidget , public Ui::source_editor_area
{
    Q_OBJECT
        
        public:
        
        dlgSourceEditorArea(QWidget*);
        THighlighter * highlighter;
    //QsciLexerLua * mpLuaLexer;
    
signals:
    
    
public slots:
    
    
};

#endif

