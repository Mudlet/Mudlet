#include "dlgCrashReportConsent.h"
#include "mudlet.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSettings>
#include <QTextEdit>

dlgCrashReportConsent::dlgCrashReportConsent(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Crash Reporting"));
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel(tr("Mudlet crashed during the last session. "
                                  "Would you like to send a crash report to help us fix the issue? "
                                  "You can also add optional comments below."));
    label->setWordWrap(true);
    layout->addWidget(label);

    mFeedbackTextEdit = new QTextEdit;
    mFeedbackTextEdit->setPlaceholderText(tr("You can add any details about what you were doing when Mudlet crashed here."));
    layout->addWidget(mFeedbackTextEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
    buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Send Report"));
    buttonBox->button(QDialogButtonBox::No)->setText(tr("Don't Send"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    setLayout(layout);
    resize(400, 300);
}

dlgCrashReportConsent::~dlgCrashReportConsent()
{
}

QString dlgCrashReportConsent::getFeedback() const
{
    return mFeedbackTextEdit->toPlainText();
}
