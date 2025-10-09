#include "WideComboBox.h"
#include <QScrollBar>
#include <QAbstractItemView>

void WideComboBox::showPopup() {
    // compute width of widest content.
    int maxWidth = view()->sizeHintForColumn(0);
    if (maxWidth > 0) {
        maxWidth += view()->verticalScrollBar()->sizeHint().width();
    }

    // temporarily adjust popup width.
    view()->setMinimumWidth(maxWidth);

    QComboBox::showPopup();
}
