#ifndef COCKATRICE_CSS_PARSER_H
#define COCKATRICE_CSS_PARSER_H

#include <QString>
#include <QWidget>

// ---------------------------------------------------------------------------
// CssParser
//
// Owns the in-memory rule list parsed from a Qt stylesheet.
// Has no UI dependencies — safe to unit-test standalone.
// ---------------------------------------------------------------------------
class CssParser
{
public:
    struct Rule
    {
        QString selector;
        QString body;
        int line = 0;
    };

    // Parse raw stylesheet text into rules[].
    // Strips C-style comments, splits comma-grouped selectors.
    void parse(const QString &stylesheetText);

    // Returns true if `w` is matched by `selector`.
    // Handles: TypeName, #objectName, :pseudo, ::subcontrol, combinations.
    bool selectorMatches(QWidget *w, const QString &selector) const;

    // Direct access to the parsed rule list.
    QVector<Rule> &rules()
    {
        return m_rules;
    }
    const QVector<Rule> &rules() const
    {
        return m_rules;
    }

private:
    // Returns true if `w` currently satisfies `pseudo` (e.g. "hover", "checked").
    // Unknown pseudo-states return true (conservative / non-filtering).
    bool widgetHasPseudo(QWidget *w, const QString &pseudo) const;

    // Returns true if the widget type supports `sub` as a valid subcontrol.
    bool widgetHasSubcontrol(QWidget *w, const QString &sub) const;

    QVector<Rule> m_rules;
};

#endif // COCKATRICE_CSS_PARSER_H
