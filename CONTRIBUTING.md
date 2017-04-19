# Feature Requests
Want a new feature in Mudlet? Let us know about it! We also have a long-running feature requests thread on [our forums](http://forums.mudlet.org/viewtopic.php?f=5&t=92).

# Bug Reports
We're sorry you've encountered an issue with Mudlet and appreciate that you're taking the time to report it to us. 

To help us find & fix the problem quickest, please include:
* as much information possible about the issue
* your Mudlet version as it says in the ``About`` window
* specific instructions on how to reproduce the issue if you know how to

# Coding guidelines

If you're a first-timer, you're excluded, we'll go easy on you :wink:

## Use QLatin1String over QStringLiteral if possible

Some methods in the Qt API have overloads for either taking a QString, or a QLatin1String object.
This is because Latin1 is simpler to parse than UTF-16, and therefore the QLatin1String version can
be faster, and use less memory. This is worth remembering especially for some QString methods:

```cpp
bool same = (str == QLatin1String("Hello"));
str.startsWith(QLatin1String("Hello"));
str += QLatin1String("World");
```

are more efficient than

```cpp
bool same = (str == QStringLiteral("Hello"));
str.startsWith(QStringLiteral("Hello"));
str += QStringLiteral("World");
```

([source](http://blog.qt.io/blog/2014/06/13/qt-weekly-13-qstringliteral/),
 [additional reading](https://woboq.com/blog/qstringliteral.html))

## Do not use ``QStringLiteral("")``

Prefer ``QString()`` over ``QStringLiteral("")`` for  for empty strings - the default constructor 
for QString is cheaper in terms of both instructions and memory.

([source](http://blog.qt.io/blog/2014/06/13/qt-weekly-13-qstringliteral/))

## Avoid duplicated QStringLiterals

Avoid having multiple QStringLiterals with the same content. For plain literals and QLatin1String, compilers
try to consolidate identical literals so that they are not duplicated. For QStringLiteral, identical strings
cannot be merged.

([source](http://blog.qt.io/blog/2014/06/13/qt-weekly-13-qstringliteral/))

# Internationalization do's and don'ts

Do:
* enable strings visible in the Mudlet GUI to be translateable

Don't:
* translate Mudlet API functions, events, or error messages
* use numbers in the API - English words are preferred instead
* try to assemble a sentence on the fly - English grammar does not translate into other languages. Present the full sentence to translators instead
