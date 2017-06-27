#ifndef TSCRIPTEDITORMANAGER_H
#define TSCRIPTEDITORMANAGER_H

#include "edbee/edbee.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/texttheme.h"
#include "edbee/models/textgrammar.h"

class TScriptEditorManager
{
public:
    TScriptEditorManager();
    void applyConfigToWidget(edbee::TextEditorWidget* widget) const;
private:
    edbee::Edbee* _edbee;
    edbee::TextGrammar* _grammar;
    QString _themeName;
};

#endif // TSCRIPTEDITORMANAGER_H
