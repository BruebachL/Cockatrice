#ifndef DLG_FORGOTPASSWORDREQUEST_H
#define DLG_FORGOTPASSWORDREQUEST_H

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

class QLabel;
class QPushButton;
class QCheckBox;

class DlgForgotPasswordRequest : public QDialog
{
    Q_OBJECT
public:
    explicit DlgForgotPasswordRequest(QWidget *parent = nullptr);
    QString getHost() const
    {
        return hostEdit->text();
    }
    int getPort() const
    {
        return portEdit->text().toInt();
    }
    QString getPlayerName() const
    {
        return playernameEdit->text();
    }
private slots:
    void actOk();

private:
    QLabel *infoLabel, *hostLabel, *portLabel, *playernameLabel;
    QLineEdit *hostEdit, *portEdit, *playernameEdit;
};

#endif
