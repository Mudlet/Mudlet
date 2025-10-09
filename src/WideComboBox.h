#ifndef WIDECOMBOBOX_H
#define WIDECOMBOBOX_H

#include <QComboBox>
#include <QObject>

class WideComboBox : public QComboBox {
    Q_OBJECT
public:
    using QComboBox::QComboBox; // inherit constructors

protected:
    void showPopup() override;
};

#endif // WIDECOMBOBOX_H
