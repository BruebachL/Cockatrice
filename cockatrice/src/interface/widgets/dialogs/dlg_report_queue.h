#ifndef COCKATRICE_DLG_REPORT_QUEUE_H
#define COCKATRICE_DLG_REPORT_QUEUE_H

#include <QDialog>
#include <QList>
#include <libcockatrice/protocol/pb/response.pb.h>
#include <libcockatrice/protocol/pb/serverinfo_report.pb.h>

class AbstractClient;
class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

class DlgReportQueue : public QDialog
{
    Q_OBJECT
public:
    explicit DlgReportQueue(AbstractClient *_client, QWidget *parent = nullptr);

private slots:
    void refreshList();
    void reportListResponse(const Response &response);
    void assignReport();
    void assignResponse(const Response &response);
    void resolveReport(bool dismissed);
    void resolveResponse(const Response &response);
    void onSelectionChanged();

private:
    int selectedReportId() const;
    void setActionsEnabled(bool enabled);
    void updateActionStates();

    AbstractClient *client;

    QCheckBox *unresolvedOnlyBox;
    QPushButton *refreshButton;
    QTableWidget *table;
    QTextEdit *descriptionEdit;
    QPushButton *assignButton;
    QPushButton *resolveButton;
    QPushButton *dismissButton;
    QLabel *statusLabel;

    QList<ServerInfo_Report> currentReports;
};

#endif // COCKATRICE_DLG_REPORT_QUEUE_H
