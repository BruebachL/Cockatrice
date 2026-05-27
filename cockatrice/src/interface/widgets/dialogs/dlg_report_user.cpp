#include "dlg_report_user.h"

#include "abstract_client.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <libcockatrice/protocol/pb/command_report.pb.h>
#include <libcockatrice/protocol/pending_command.h>

DlgReportUser::DlgReportUser(AbstractClient *_client, const QString &_reportedUser, int _gameId, QWidget *parent)
    : QDialog(parent), client(_client), reportedUser(_reportedUser), gameId(_gameId)
{
    setWindowTitle(tr("Report User"));
    setMinimumWidth(420);

    auto *form = new QFormLayout;
    form->addRow(tr("Reporting:"), new QLabel(reportedUser));

    categoryBox = new QComboBox;
    categoryBox->addItem(tr("Cheating / Unsporting behavior"), "cheating");
    categoryBox->addItem(tr("Harassment / Abuse"), "harassment");
    categoryBox->addItem(tr("Hate speech"), "hate_speech");
    categoryBox->addItem(tr("Spam"), "spam");
    categoryBox->addItem(tr("Other"), "other");
    form->addRow(tr("Category:"), categoryBox);

    descriptionEdit = new QTextEdit;
    descriptionEdit->setPlaceholderText(tr("Please describe what happened..."));
    descriptionEdit->setFixedHeight(100);
    form->addRow(tr("Description:"), descriptionEdit);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Submit Report"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgReportUser::actSubmit);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttonBox);
}

void DlgReportUser::actSubmit()
{
    const QString description = descriptionEdit->toPlainText().trimmed();
    if (description.isEmpty()) {
        QMessageBox::warning(this, tr("Missing description"), tr("Please describe what happened before submitting."));
        return;
    }

    buttonBox->setEnabled(false);

    Command_Report cmd;
    cmd.set_reported_user(reportedUser.toStdString());
    cmd.set_category(categoryBox->currentData().toString().toStdString());
    cmd.set_description(description.toStdString());
    if (gameId >= 0) {
        cmd.set_game_id(gameId);
    }

    PendingCommand *pend = client->prepareSessionCommand(cmd);
    connect(pend, &PendingCommand::finished, this, &DlgReportUser::reportResponse);
    client->sendCommand(pend);
}

void DlgReportUser::reportResponse(const Response &response)
{
    buttonBox->setEnabled(true);
    if (response.response_code() == Response::RespOk) {
        QMessageBox::information(this, tr("Report Submitted"), tr("Your report has been submitted. Thank you."));
        accept();
    } else {
        QMessageBox::warning(this, tr("Submission Failed"), tr("Failed to submit report. Please try again."));
    }
}