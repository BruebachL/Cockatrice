#ifndef FIRST_RUN_WIZARD_PAGE_H
#define FIRST_RUN_WIZARD_PAGE_H

#include <QWidget>

/**
 * Base class for a single step of FirstRunWizard.
 *
 * QWidget-based rather than QWizardPage-based: FirstRunWizard is a
 * QDialog + QStackedWidget shell (not a QWizard) so it can own the
 * banner/step-dot chrome that QWizard's native styles don't give us
 * consistent control over. Naming mirrors OracleWizardPage for
 * familiarity only -- the two hierarchies are unrelated.
 */
class FirstRunWizardPage : public QWidget
{
    Q_OBJECT

public:
    explicit FirstRunWizardPage(QWidget *parent = nullptr) : QWidget(parent)
    {
    }

    //! Called every time the page becomes visible, including navigating back to it.
    virtual void initializePage()
    {
    }

    //! Called before advancing past this page. Return false to block navigation;
    //! the page itself is responsible for telling the user why.
    virtual bool validatePage()
    {
        return true;
    }

    //! Whether Next/Finish should currently be enabled. Pages doing async work
    //! can flip this mid-step; emit completeChanged() when they do.
    virtual bool isComplete() const
    {
        return true;
    }

    //! Whether the wizard's "Skip" button should be offered on this page.
    virtual bool isSkippable() const
    {
        return false;
    }

    virtual QString stepTitle() const = 0;
    virtual QString stepSubtitle() const
    {
        return {};
    }

    virtual void retranslateUi() = 0;

    signals:
        void completeChanged();
};

#endif // FIRST_RUN_WIZARD_PAGE_H