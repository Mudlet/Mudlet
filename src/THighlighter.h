#ifndef THIGHLIGHTER_H
#define THIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

class QTextDocument;

class THighlighter : public QSyntaxHighlighter
{
     Q_OBJECT

 public:
                               THighlighter(QTextDocument *parent = 0);
     void                      setSearchPattern( QString p );


 protected:
     void                      highlightBlock(const QString &text);

 private:
     struct HighlightingRule
     {
         QRegExp pattern;
         QTextCharFormat format;
     };
     QString                   mSearchPattern;
     QVector<HighlightingRule> highlightingRules;
     QRegExp                   commentStartExpression;
     QRegExp                   commentEndExpression;
     QRegExp                   stringStart;
     QRegExp                   stringEnd;
     QTextCharFormat           keywordFormat;
     QTextCharFormat           searchFormat;
     QTextCharFormat           classFormat;
     QTextCharFormat           singleLineCommentFormat;
     QTextCharFormat           multiLineCommentFormat;
     QTextCharFormat           quotationFormat;
     QTextCharFormat           functionFormat;
     //bool isString;
 };

#endif // THIGHLIGHTER_H
