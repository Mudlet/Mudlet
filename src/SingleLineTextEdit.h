// SingleLineTextEdit.h
#ifndef SINGLELINETEXTEDIT_H
#define SINGLELINETEXTEDIT_H

#include <QTextEdit>

class SingleLineTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit SingleLineTextEdit(QWidget *parent = nullptr);

signals:
    // Signal similar to QLineEdit's editingFinished signal
    void editingFinished();

protected:
    // Overrides to handle key events and line wrapping
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // SINGLELINETEXTEDIT_H
