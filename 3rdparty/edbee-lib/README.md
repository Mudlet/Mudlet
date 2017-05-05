edbee-lib
=========

Edbee is a Qt based Editor Component. 
It supports Multiple-carets, Textmate Scoping/Grammar and Highlighting support.

The base class of edbee is a QWidget, it has been written from scratch and is not based on the Qt default editor components. 

![Screenshot of the example application](http://edbee.net/images/screenshot1.png)

The main website for edbee is at http://edbee.net/
You can find the generated documentation at http://docs.edbee.net/

This repository is a clean extraction of the previous edbee monolithical repository. 
This library doesn't contain any data files. Using it should become as easy as the following example:

```C++
#include "edbee/edbee.h"
edbee::TextEditorWidget* widget =  new edbee::TextEditorWidget();
```

Unfortunately we're not at this point yet. Currently the edbee library will not function without loading a default keymap file.


Examples
--------

Using the component is pretty easy. First you must setup the edbee environment.
This process is required to make the library know the location of the settings
files:

```C++
#include "edbee/edbee.h"

// get the edbee instance
edbee::Edbee* tm = edbee::Edbee::instance();

// configure your paths
tm->setKeyMapPath( "location/of/your/keymap/");
tm->setGrammarPath(  "location/of/your/syntaxfiles/"" );
tm->setThemePath( "location/of/your/themes/" );

// initialize the library
tm->init();   

// by registering a shutdown function on application exit you don't have to destroy the edbee
// resources yourself. It will result in a clean shutdown
tm->autoShutDownOnAppExit();
```



After that you're ready to go.
You can create a widget like this:

```C++
edbee::TextEditorWidget* widget =  new edbee::TextEditorWidget();
```

Of course it would also be nice to fill the editor with a file. you can use the included serializer for this.

```C++
#include "edbee/io/textdocumentserializer.h"
#include "edbee/texteditorwidget.h"

edbee::TextEditorWidget* widget =  new edbee::TextEditorWidget();
edbee::TextDocumentSerializer serializer( widget->textDocument() );
if( !serializer.load( "your-filename.rb" ) ) {
    QMessageBox::warning(this, tr("Error opening file"), tr("Error opening file!\n%1").arg(serializer.errorString()) );
}

```

After loading the textfile it is nice to detect the grammar/language of this file.
The edbee library uses an extension based file-type detection. Of course you can also plugin your own.

```C++
#include "edbee/edbee.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textgrammar.h"
#include "edbee/texteditorwidget.h"

TextGrammarManager* grammarManager = edbee::Edbee::instance()->grammarManager();
TextGrammar* grammar = grammarManager->detectGrammarWithFilename( "a-ruby-file.rb" );
widget->textDocument()->setLanguagGrammar( grammar );

```

Tips and Tricks
----------------

### Object-name conflicts

When you're QT projects uses for example util.cpp you can get an object-file collission. (Makefile: Warning: overriding commands for target util.o). A workaround for this is appending the following lines in your 
(OBJECTS_DIR is optional, but prevents a warning when running QMAKE)

```
CONFIG += object_parallel_to_source in your .pro file
OBJECTS_DIR = objects
```


Known Issues and Missing Features
---------------------------------

* The editor doesn't support word-wrapping. (yet)
* It has issues with long lines. The cause of this is the nature of QTextLayout and the support of variable font sizes. In the future this can be fixed for monospaced fonts.
* Optimalisations for better render support and background calculate/paint-ahead functionality
* I really want to build in scripting support, for extending the editor with plugins. 


Contributing
------------

You can contribute via github
- Fork it
- Create your feature branch (git checkout -b my-new-feature)
- Commit your changes (git commit -am 'Added some feature')
- Push to the branch (git push origin my-new-feature)
- Create new Pull Request

Of course you can also contribute by contacting me via twitter @gamecreature or drop me al message 
via the email-address supplied at [https://github.com/gamecreature](https://github.com/gamecreature)

Issues?
-------

Though we have our own issue-tracker at ( http://issues.edbee.net/ ), you can report your problems 
via the github issue tracker or send me a message [https://github.com/gamecreature](https://github.com/gamecreature)




