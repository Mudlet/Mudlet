#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class TriggerHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit TriggerHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat characterClassFormat;
    QTextCharFormat quantifierFormat;
    QTextCharFormat groupFormat;
    QTextCharFormat anchorFormat;
    QTextCharFormat escapeCharFormat;
    QTextCharFormat spaceCharFormat;

    void initialiseRules();
};
