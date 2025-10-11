#ifndef DLCRASHREPORTCONSENT_H
#define DLCRASHREPORTCONSENT_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class dlgCrashReportConsent : public QDialog
{
    Q_OBJECT

public:
    explicit dlgCrashReportConsent(QWidget *parent = nullptr);
    ~dlgCrashReportConsent() override;

    QString getFeedback() const;

private:
    QTextEdit *mFeedbackTextEdit;
};

#endif // DLCRASHREPORTCONSENT_H
