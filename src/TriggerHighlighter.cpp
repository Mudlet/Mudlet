#include "TriggerHighlighter.h"
TriggerHighlighter::TriggerHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    initialiseRules();
}

// Processing starts from top to bottom, so put higher priority colours last.
void TriggerHighlighter::initialiseRules()
{
    // TODO: does this work ok with dark theme?
    // a muted pastel colour range
    QColor red(255, 239, 239);
    QColor orange(255,223,186,255);
    QColor yellow(255,255,186,255);
    QColor green(186,255,201,255);
    QColor blue(186,225,255,255);
    QColor gray(241, 241, 241);

    // (pattern), (?:pattern), (?=pattern) (?<capture>)
    groupFormat.setForeground(Qt::darkGreen);
    groupFormat.setBackground(QBrush(green));
    groupFormat.setFontWeight(QFont::Bold);
    //highlightingRules.append({QRegularExpression(R"(\([?][=:!<]|\\\(|\>|\))"), groupFormat});
    highlightingRules.append({QRegularExpression(R"(\(.*\))"), groupFormat});
    //highlightingRules.append({QRegularExpression(R"(\(|\))"), groupFormat});

    // ^ $ ? \b \B
    anchorFormat.setForeground(Qt::red);
    anchorFormat.setBackground(QBrush(red));
    anchorFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"(\^|\$|\?|\\[bB])"), anchorFormat});

    // * + ? . [a-z] {m,n}
    quantifierFormat.setForeground(Qt::black);
    quantifierFormat.setBackground(QBrush(orange));
    quantifierFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"([*+.]|{[0-9,]*})"), quantifierFormat});
    highlightingRules.append({QRegularExpression(R"((\\[dDsSwW]|\[[^\]]+\]))"), quantifierFormat});

    //
    escapeCharFormat.setForeground(Qt::black);
    escapeCharFormat.setBackground(yellow);
    escapeCharFormat.setFontWeight(QFont::Bold);
    highlightingRules.append({QRegularExpression(R"(\\[nrtvfae]|\\[xXuU][0-9A-Fa-f]+|\\[0-7]{1,3})"), escapeCharFormat});

    spaceCharFormat.setBackground(gray);
    highlightingRules.append({QRegularExpression(R"(\s+)"), spaceCharFormat});
}

void TriggerHighlighter::highlightBlock(const QString &text)
{
    // with states, we should be able to track which trigger type is selected and just turn off
    // highlighting when we're not a Regex type trigger
//    if (this->currentBlockState() < 0) {
//        return;
//    }
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
