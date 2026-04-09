#include "css_parser.h"

#include "theme_inspector_widget.h"

#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>

// ---------------------------------------------------------------------------
// parse
// ---------------------------------------------------------------------------
void CssParser::parse(const QString &stylesheetText)
{
    m_rules.clear();

    QString text = stylesheetText;
    // Strip C-style comments
    text.remove(QRegularExpression(R"(/\*[\s\S]*?\*/)"));

    QRegularExpression re(R"(([^\{]+)\{([^\}]*)\})");
    int line = 1;

    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        QString body = m.captured(2).trimmed();

        // Split comma-grouped selectors into individual rules
        for (QString sel : m.captured(1).split(',', Qt::SkipEmptyParts))
            m_rules.push_back({sel.trimmed(), body, line});

        line += m.captured(0).count('\n');
    }
}

// ---------------------------------------------------------------------------
// selectorMatches
// ---------------------------------------------------------------------------
bool CssParser::selectorMatches(QWidget *w, const QString &s) const
{
    if (!w || s.isEmpty())
        return false;

    QRegularExpression re(R"(^(?<type>[\w]+)?(#(?<obj>[\w-]+))?(?::(?<pseudo>[\w-]+))?(?:::(?<sub>[\w-]+))?$)");
    QRegularExpressionMatch m = re.match(s.trimmed());
    if (!m.hasMatch())
        return false;

    const QString selType = m.captured("type");
    const QString selObj = m.captured("obj");
    const QString selPseudo = m.captured("pseudo");
    const QString selSub = m.captured("sub");

    // Type — walk the inheritance chain so e.g. "QWidget" matches QPushButton
    if (!selType.isEmpty()) {
        const QMetaObject *mo = w->metaObject();
        bool found = false;
        while (mo) {
            if (selType == mo->className()) {
                found = true;
                break;
            }
            mo = mo->superClass();
        }
        if (!found)
            return false;
    }

    // Object name
    if (!selObj.isEmpty() && w->objectName() != selObj)
        return false;

    // Pseudo-state
    if (!widgetHasPseudo(w, selPseudo))
        return false;

    // Subcontrol
    if (!widgetHasSubcontrol(w, selSub))
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// widgetHasPseudo
// ---------------------------------------------------------------------------
bool CssParser::widgetHasPseudo(QWidget *w, const QString &pseudo) const
{
    if (pseudo.isEmpty())
        return true;

    if (pseudo == "hover")
        return w->underMouse();
    if (pseudo == "disabled")
        return !w->isEnabled();
    if (pseudo == "enabled")
        return w->isEnabled();
    if (pseudo == "focus")
        return w->hasFocus();
    if (pseudo == "visible")
        return w->isVisible();
    if (pseudo == "hidden")
        return !w->isVisible();

    if (pseudo == "checked") {
        if (auto *cb = qobject_cast<QCheckBox *>(w))
            return cb->isChecked();
        if (auto *rb = qobject_cast<QRadioButton *>(w))
            return rb->isChecked();
        if (auto *btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && btn->isChecked();
    }
    if (pseudo == "unchecked") {
        if (auto *cb = qobject_cast<QCheckBox *>(w))
            return !cb->isChecked();
        if (auto *rb = qobject_cast<QRadioButton *>(w))
            return !rb->isChecked();
        if (auto *btn = qobject_cast<QPushButton *>(w))
            return btn->isCheckable() && !btn->isChecked();
    }

    // Unknown pseudo — conservative: don't filter it out
    return true;
}

// ---------------------------------------------------------------------------
// widgetHasSubcontrol
// ---------------------------------------------------------------------------
bool CssParser::widgetHasSubcontrol(QWidget *w, const QString &sub) const
{
    if (sub.isEmpty())
        return true;

    const QString type = w->metaObject()->className();
    if (!QtSelectors::WIDGET_VALID_SUBCONTROLS.contains(type))
        return false;

    return QtSelectors::WIDGET_VALID_SUBCONTROLS[type].contains("::" + sub);
}