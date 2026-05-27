#include "dlg_report_queue.h"

#include "abstract_client.h"

#include <QCheckBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <libcockatrice/protocol/pb/command_report_assign.pb.h>
#include <libcockatrice/protocol/pb/command_report_list.pb.h>
#include <libcockatrice/protocol/pb/command_report_resolve.pb.h>
#include <libcockatrice/protocol/pb/response_report_list.pb.h>
#include <libcockatrice/protocol/pending_command.h>

namespace
{
constexpr int COL_ID = 0;
constexpr int COL_TIME = 1;
constexpr int COL_REPORTER = 2;
constexpr int COL_REPORTED = 3;
constexpr int COL_CATEGORY = 4;
constexpr int COL_GAMEID = 5;
constexpr int COL_REPLAYID = 6;
constexpr int COL_STATUS = 7;
constexpr int COL_ASSIGNED = 8;
constexpr int COL_COUNT = 9;
} // namespace

DlgReportQueue::DlgReportQueue(AbstractClient *_client, QWidget *parent) : QDialog(parent), client(_client)
{
    setWindowTitle(tr("Report Queue"));
    setMinimumSize(850, 560);

    // Top bar
    unresolvedOnlyBox = new QCheckBox(tr("Unresolved only"));
    unresolvedOnlyBox->setChecked(true);
    connect(unresolvedOnlyBox, &QCheckBox::checkStateChanged, this, &DlgReportQueue::refreshList);

    refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, &QPushButton::clicked, this, &DlgReportQueue::refreshList);

    auto *topBar = new QHBoxLayout;
    topBar->addWidget(unresolvedOnlyBox);
    topBar->addStretch();
    topBar->addWidget(refreshButton);

    // Table
    table = new QTableWidget(0, COL_COUNT);
    table->setHorizontalHeaderLabels({tr("#"), tr("Time"), tr("Reporter"), tr("Reported User"), tr("Category"),
                                      tr("Game ID"), tr("Replay ID"), tr("Status"), tr("Assigned To")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setSectionResizeMode(COL_TIME, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(COL_REPORTED, QHeaderView::Stretch);
    connect(table, &QTableWidget::itemSelectionChanged, this, &DlgReportQueue::onSelectionChanged);

    // Description area
    auto *descGroup = new QGroupBox(tr("Description"));
    descriptionEdit = new QTextEdit;
    descriptionEdit->setReadOnly(true);
    descriptionEdit->setFixedHeight(80);
    auto *descLayout = new QVBoxLayout(descGroup);
    descLayout->setContentsMargins(4, 4, 4, 4);
    descLayout->addWidget(descriptionEdit);

    // Action buttons
    assignButton = new QPushButton(tr("Assign to Me"));
    resolveButton = new QPushButton(tr("Resolve..."));
    dismissButton = new QPushButton(tr("Dismiss..."));
    connect(assignButton, &QPushButton::clicked, this, &DlgReportQueue::assignReport);
    connect(resolveButton, &QPushButton::clicked, this, [this]() { resolveReport(false); });
    connect(dismissButton, &QPushButton::clicked, this, [this]() { resolveReport(true); });

    statusLabel = new QLabel;

    auto *actionBar = new QHBoxLayout;
    actionBar->addWidget(assignButton);
    actionBar->addWidget(resolveButton);
    actionBar->addWidget(dismissButton);
    actionBar->addStretch();
    actionBar->addWidget(statusLabel);

    auto *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *bottomBar = new QHBoxLayout;
    bottomBar->addLayout(actionBar);
    bottomBar->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topBar);
    layout->addWidget(table, 1);
    layout->addWidget(descGroup);
    layout->addLayout(bottomBar);

    setActionsEnabled(false);
    refreshList();
}

void DlgReportQueue::refreshList()
{
    refreshButton->setEnabled(false);
    statusLabel->setText(tr("Loading..."));
    table->setRowCount(0);
    currentReports.clear();
    descriptionEdit->clear();
    setActionsEnabled(false);

    Command_ReportList cmd;
    cmd.set_unresolved_only(unresolvedOnlyBox->isChecked());

    PendingCommand *pend = client->prepareModeratorCommand(cmd);
    connect(pend, &PendingCommand::finished, this, &DlgReportQueue::reportListResponse);
    client->sendCommand(pend);
}

void DlgReportQueue::reportListResponse(const Response &response)
{
    refreshButton->setEnabled(true);

    if (response.response_code() != Response::RespOk) {
        statusLabel->setText(tr("Failed to load reports."));
        return;
    }

    const Response_ReportList &resp = response.GetExtension(Response_ReportList::ext);
    currentReports.clear();
    for (int i = 0; i < resp.reports_size(); ++i) {
        currentReports.append(resp.reports(i));
    }

    table->setSortingEnabled(false);
    table->setRowCount(currentReports.size());

    for (int row = 0; row < currentReports.size(); ++row) {
        const ServerInfo_Report &r = currentReports[row];

        auto *idItem = new QTableWidgetItem(QString::number(r.report_id()));
        idItem->setData(Qt::UserRole, r.report_id());
        table->setItem(row, COL_ID, idItem);

        const QDateTime dt = QDateTime::fromSecsSinceEpoch(r.report_time());
        table->setItem(row, COL_TIME, new QTableWidgetItem(dt.toString("yyyy-MM-dd hh:mm")));
        table->setItem(row, COL_REPORTER, new QTableWidgetItem(QString::fromStdString(r.reporter_name())));
        table->setItem(row, COL_REPORTED, new QTableWidgetItem(QString::fromStdString(r.reported_user_name())));
        table->setItem(row, COL_GAMEID, new QTableWidgetItem(QString::number(r.game_id())));
        table->setItem(row, COL_REPLAYID, new QTableWidgetItem(QString::number(r.replay_id())));
        table->setItem(row, COL_CATEGORY, new QTableWidgetItem(QString::fromStdString(r.category())));
        table->setItem(row, COL_STATUS, new QTableWidgetItem(QString::fromStdString(r.status())));
        table->setItem(row, COL_ASSIGNED, new QTableWidgetItem(QString::fromStdString(r.assigned_mod_name())));
    }

    table->setSortingEnabled(true);
    table->resizeColumnsToContents();
    table->horizontalHeader()->setSectionResizeMode(COL_REPORTED, QHeaderView::Stretch);

    statusLabel->setText(tr("%1 report(s)").arg(currentReports.size()));
}

void DlgReportQueue::onSelectionChanged()
{
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, COL_ID)) {
        descriptionEdit->clear();
        setActionsEnabled(false);
        return;
    }

    // Use stored ID to find the report — safe even after column sorting
    const int reportId = table->item(row, COL_ID)->data(Qt::UserRole).toInt();
    for (const ServerInfo_Report &r : currentReports) {
        if (r.report_id() == reportId) {
            descriptionEdit->setPlainText(QString::fromStdString(r.description()));
            break;
        }
    }

    updateActionStates();
}

void DlgReportQueue::updateActionStates()
{
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, COL_STATUS)) {
        setActionsEnabled(false);
        return;
    }

    const QString status = table->item(row, COL_STATUS)->text();
    assignButton->setEnabled(status == "open");
    resolveButton->setEnabled(status == "open" || status == "assigned");
    dismissButton->setEnabled(status == "open" || status == "assigned");
}

void DlgReportQueue::setActionsEnabled(bool enabled)
{
    assignButton->setEnabled(enabled);
    resolveButton->setEnabled(enabled);
    dismissButton->setEnabled(enabled);
}

int DlgReportQueue::selectedReportId() const
{
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, COL_ID)) {
        return -1;
    }
    return table->item(row, COL_ID)->data(Qt::UserRole).toInt();
}

void DlgReportQueue::assignReport()
{
    const int reportId = selectedReportId();
    if (reportId < 0) {
        return;
    }

    setActionsEnabled(false);
    statusLabel->setText(tr("Assigning..."));

    Command_ReportAssign cmd;
    cmd.set_report_id(reportId);

    PendingCommand *pend = client->prepareModeratorCommand(cmd);
    connect(pend, &PendingCommand::finished, this, &DlgReportQueue::assignResponse);
    client->sendCommand(pend);
}

void DlgReportQueue::assignResponse(const Response &response)
{
    if (response.response_code() == Response::RespOk) {
        statusLabel->setText(tr("Assigned."));
        refreshList();
    } else {
        statusLabel->setText(tr("Assignment failed."));
        updateActionStates();
    }
}

void DlgReportQueue::resolveReport(bool dismissed)
{
    const int reportId = selectedReportId();
    if (reportId < 0) {
        return;
    }

    bool ok;
    const QString note = QInputDialog::getText(this, dismissed ? tr("Dismiss Report") : tr("Resolve Report"),
                                               dismissed ? tr("Optional note:") : tr("Resolution note (optional):"),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }

    setActionsEnabled(false);
    statusLabel->setText(dismissed ? tr("Dismissing...") : tr("Resolving..."));

    Command_ReportResolve cmd;
    cmd.set_report_id(reportId);
    cmd.set_dismissed(dismissed);
    if (!note.isEmpty()) {
        cmd.set_resolution_note(note.toStdString());
    }

    PendingCommand *pend = client->prepareModeratorCommand(cmd);
    connect(pend, &PendingCommand::finished, this, &DlgReportQueue::resolveResponse);
    client->sendCommand(pend);
}

void DlgReportQueue::resolveResponse(const Response &response)
{
    if (response.response_code() == Response::RespOk) {
        statusLabel->setText(tr("Done."));
        refreshList();
    } else {
        statusLabel->setText(tr("Action failed."));
        updateActionStates();
    }
}