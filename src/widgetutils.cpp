/***************************************************************************
 *   Copyright (C) 2026 by Andrew Johnson - andrew@johnson5.net            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "widgetutils.h"

#include "utils.h"

#include <QEvent>
#include <QPointer>
#include <QRegularExpression>
#include <QTextDocument>

namespace {

// The description this filter last gave a widget, which is how a later
// tooltip change tells its own description apart from one set elsewhere
constexpr auto csmDerivedDescription = "mudletDescriptionFromToolTip";

class ToolTipDescriptionSync : public QObject
{
public:
    using QObject::QObject;

    bool eventFilter(QObject* pObject, QEvent* pEvent) override
    {
        if (pEvent->type() == QEvent::ToolTipChange && pObject->isWidgetType()) {
            update(static_cast<QWidget*>(pObject));
        }
        return QObject::eventFilter(pObject, pEvent);
    }

private:
    void update(QWidget* pWidget)
    {
        const QString current = pWidget->accessibleDescription();
        if (!current.isEmpty() && current != pWidget->property(csmDerivedDescription).toString()) {
            return;
        }

        // Qt::mightBeRichText() is the test QToolTip uses to pick a format. A
        // tooltip that fails it is shown and read as it is, and parsing it as
        // HTML here would drop any literal "<...>" or "&...;" in it.
        const QString toolTip = pWidget->toolTip();
        const QString description = Qt::mightBeRichText(toolTip) ? plainText(toolTip) : QString();
        pWidget->setProperty(csmDerivedDescription, description);
        pWidget->setAccessibleDescription(description);
    }

    QString plainText(const QString& html)
    {
        mDocument.setHtml(html);
        // Block elements come out as separate lines, which read better as one
        // announcement with a space between them
        return mDocument.toPlainText().replace(scmWhitespaceRun, qsl(" ")).trimmed();
    }

    inline static const QRegularExpression scmWhitespaceRun{qsl(R"(\s+)")};
    QTextDocument mDocument;
};

} // namespace

void widgetutils::syncAccessibleDescriptionsWithToolTips()
{
    static QPointer<ToolTipDescriptionSync> spSync;
    if (spSync || !qApp) {
        return;
    }
    spSync = new ToolTipDescriptionSync(qApp);
    qApp->installEventFilter(spSync);
}
