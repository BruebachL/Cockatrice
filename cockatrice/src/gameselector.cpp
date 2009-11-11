#include <QtGui>
#include "gameselector.h"
#include "dlg_creategame.h"
#include "gamesmodel.h"

GameSelector::GameSelector(Client *_client, QWidget *parent)
	: QWidget(parent), client(_client)
{
	gameListView = new QTreeView;
	gameListModel = new GamesModel(this);
	gameListProxyModel = new GamesProxyModel(this);
	gameListProxyModel->setSourceModel(gameListModel);
	gameListView->setModel(gameListProxyModel);

	showFullGamesCheckBox = new QCheckBox;
	createButton = new QPushButton;
	joinButton = new QPushButton;
	spectateButton = new QPushButton;
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(showFullGamesCheckBox);
	buttonLayout->addStretch();
	buttonLayout->addWidget(createButton);
	buttonLayout->addWidget(joinButton);
	buttonLayout->addWidget(spectateButton);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(gameListView);
	mainLayout->addLayout(buttonLayout);

	retranslateUi();
	setLayout(mainLayout);

	setMinimumWidth(gameListView->columnWidth(0) * gameListModel->columnCount());
	setMinimumHeight(400);

	connect(showFullGamesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showFullGamesChanged(int)));
	connect(createButton, SIGNAL(clicked()), this, SLOT(actCreate()));
	connect(joinButton, SIGNAL(clicked()), this, SLOT(actJoin()));
	connect(spectateButton, SIGNAL(clicked()), this, SLOT(actJoin()));
}

void GameSelector::showFullGamesChanged(int state)
{
	gameListProxyModel->setFullGamesVisible(state);
}

void GameSelector::actCreate()
{
	DlgCreateGame dlg(client, this);
	if (dlg.exec())
		disableGameList();
}

/*void GameSelector::checkResponse(ServerResponse response)
{
	createButton->setEnabled(true);
	joinButton->setEnabled(true);
	spectateButton->setEnabled(true);

	switch (response) {
		case RespOk: disableGameList(); break;
		case RespPasswordWrong: QMessageBox::critical(this, tr("Error"), tr("Wrong password.")); break;
		case RespSpectatorsNotAllowed: QMessageBox::critical(this, tr("Error"), tr("Spectators are not allowed in this game.")); break;
		case RespContextError: QMessageBox::critical(this, tr("Error"), tr("The game is already full.")); break;
		case RespNameNotFound: QMessageBox::critical(this, tr("Error"), tr("The game does not exist any more.")); break;
		default: ;
	}
}
*/
void GameSelector::actJoin()
{
	bool spectator = sender() == spectateButton;
	
	QModelIndex ind = gameListView->currentIndex();
	if (!ind.isValid())
		return;
	const ServerGameInfo &game = gameListModel->getGame(ind.data(Qt::UserRole).toInt());
	QString password;
	if (game.getHasPassword()) {
		bool ok;
		password = QInputDialog::getText(this, tr("Join game"), tr("Password:"), QLineEdit::Password, QString(), &ok);
		if (!ok)
			return;
	}

//	PendingCommand *joinCommand = client->joinGame(game.getGameId(), password, spectator);
//	connect(joinCommand, SIGNAL(finished(ServerResponse)), this, SLOT(checkResponse(ServerResponse)));
	createButton->setEnabled(false);
	joinButton->setEnabled(false);
	spectateButton->setEnabled(false);
}

void GameSelector::enableGameList()
{
	if (isVisible())
		return;
	
	connect(client, SIGNAL(gameListEvent(const ServerGameInfo &)), gameListModel, SLOT(updateGameList(const ServerGameInfo &)));
	client->listGames();
	show();
}

void GameSelector::disableGameList()
{
	if (!isVisible())
		return;
	
	disconnect(client, 0, gameListModel, 0);
	hide();
	gameListModel->cleanList();
}

void GameSelector::retranslateUi()
{
	showFullGamesCheckBox->setText(tr("&Show full games"));
	createButton->setText(tr("C&reate"));
	joinButton->setText(tr("&Join"));
	spectateButton->setText(tr("J&oin as spectator"));
}