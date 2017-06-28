#include "TScriptEditorManager.h"

#include "edbee/models/textdocument.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"
#include "edbee/models/textgrammar.h"
#include "edbee/models/texteditorconfig.h"

#include <QDir>

TScriptEditorManager::TScriptEditorManager()
{
    _edbee = edbee::Edbee::instance();

    _edbee->autoInit();
    _edbee->autoShutDownOnAppExit();

    edbee::TextGrammarManager* grammarManager = _edbee->grammarManager();
    _grammar = grammarManager->readGrammarFile( QDir::homePath() + "/.config/mudlet/edbee/Lua.tmLanguage" );

    edbee::TextThemeManager* themeManager = _edbee->themeManager();
    themeManager->readThemeFile( QDir::homePath() + "/.config/mudlet/edbee/Fluidvisionlet.tmTheme" ); // Once it's read it becomes available

    _themeName = QLatin1Literal("Fluidvisionlet");
}


// Apply configuration to editorwidget

void TScriptEditorManager::applyConfigToWidget(edbee::TextEditorWidget* widget) const
{
    edbee::TextEditorConfig* config = widget->config();

    config->beginChanges();

    config->setSmartTab( true); // I'm not fully sure what this does, but it says "Smart" so it must be good
    config->setCaretBlinkRate( 200);

    config->setIndentSize( 2); // 2 spaces is the Lua default
    config->setFont(QFont( "Courier New", 28)); // This is just a fallback, the actual font is set later

    config->setThemeName("Fluidvisionlet");
    config->setCaretWidth( 1);

    config->setThemeName(_themeName);
    widget->textDocument()->setLanguageGrammar(_grammar);

    config->endChanges();
}
