#ifndef COCKATRICE_DLG_REPORT_USER_H
#define COCKATRICE_DLG_REPORT_USER_H

#include <QDialog>
#include <libcockatrice/protocol/pb/response.pb.h>

class AbstractClient;
class QComboBox;
class QDialogButtonBox;
class QTextEdit;

class DlgReportUser : public QDialog
{
    Q_OBJECT
public:
    DlgReportUser(AbstractClient *_client, const QString &_reportedUser, int _gameId = -1, QWidget *parent = nullptr);

private slots:
    void actSubmit();
    void reportResponse(const Response &response);

private:
    AbstractClient *client;
    QString reportedUser;
    int gameId;

    QComboBox *categoryBox;
    QTextEdit *descriptionEdit;
    QDialogButtonBox *buttonBox;
};

#endif // COCKATRICE_DLG_REPORT_USER_H
