/***************************************************************************
 *   Copyright (C) 2008 by Max-Wilhelm Bruker   *
 *   brukie@gmx.net   *
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
#include <QtGui>
//#include <QtOpenGL>

#include "window_main.h"
#include "dlg_connect.h"
#include "dlg_settings.h"
#include "gameselector.h"
#include "window_deckeditor.h"
#include "cardinfowidget.h"
#include "messagelogwidget.h"
#include "phasestoolbar.h"
#include "gameview.h"
#include "gamescene.h"
#include "player.h"
#include "game.h"
#include "carddatabase.h"
#include "zoneviewzone.h"
#include "zoneviewwidget.h"
#include "zoneviewlayout.h"
#include "chatwidget.h"

PingWidget::PingWidget(QWidget *parent)
	: QWidget(parent)
{
	setPercentage(0, -1);
}

QSize PingWidget::sizeHint() const
{
	return QSize(15, 15);
}

void PingWidget::paintEvent(QPaintEvent */*event*/)
{
	QPainter painter(this);
	QRadialGradient g(QPointF((double) width() / 2, (double) height() / 2), qMin(width(), height()) / 2.0);
	g.setColorAt(0, color);
	g.setColorAt(1, Qt::transparent);
	painter.fillRect(0, 0, width(), height(), QBrush(g));
}

void PingWidget::setPercentage(int value, int max)
{
	if (max == -1)
		color = Qt::black;
	else
		color.setHsv(120 * (1.0 - ((double) value / max)), 255, 255);
	update();
}

void MainWindow::playerAdded(Player *player)
{
	menuBar()->addMenu(player->getPlayerMenu());
	connect(player, SIGNAL(toggleZoneView(Player *, QString, int)), zoneLayout, SLOT(toggleZoneView(Player *, QString, int)));
	connect(player, SIGNAL(closeZoneView(ZoneViewZone *)), zoneLayout, SLOT(removeItem(ZoneViewZone *)));
}

void MainWindow::statusChanged(ClientStatus _status)
{
	switch (_status) {
		case StatusConnecting:
			emit logConnecting(client->peerName());
			break;
		case StatusDisconnected:
			if (game) {
				zoneLayout->clear();
				delete game;
				game = 0;
			}
			pingWidget->setPercentage(0, -1);
			aConnect->setEnabled(true);
			aDisconnect->setEnabled(false);
			aRestartGame->setEnabled(false);
			aLeaveGame->setEnabled(false);
			phasesToolbar->setActivePhase(-1);
			phasesToolbar->hide();
			gameSelector->disableGameList();
			chatWidget->disableChat();
			emit logDisconnected();
			break;
		case StatusLoggingIn:
			aConnect->setEnabled(false);
			aDisconnect->setEnabled(true);
			break;
		case StatusLoggedIn: {
/*			if (game) {
				zoneLayout->clear();
				delete game;
				game = 0;
			}
			aRestartGame->setEnabled(false);
			aLeaveGame->setEnabled(false);
			phasesToolbar->setActivePhase(-1);
			phasesToolbar->hide();
			
			view->hide();
			gameSelector->enableGameList();
			chatWidget->enableChat();
*/			break;
		}
//		case StatusPlaying: {
/*			chatWidget->disableChat();
			
			game = new Game(db, client, scene, menuBar(), this);
			connect(game, SIGNAL(hoverCard(QString)), cardInfo, SLOT(setCard(const QString &)));
			connect(game, SIGNAL(playerAdded(Player *)), this, SLOT(playerAdded(Player *)));
			connect(game, SIGNAL(playerRemoved(Player *)), scene, SLOT(removePlayer(Player *)));
			connect(game, SIGNAL(setActivePhase(int)), phasesToolbar, SLOT(setActivePhase(int)));
			connect(phasesToolbar, SIGNAL(signalDrawCard()), game, SLOT(activePlayerDrawCard()));
			connect(phasesToolbar, SIGNAL(signalUntapAll()), game, SLOT(activePlayerUntapAll()));
			messageLog->connectToGame(game);
			aRestartGame->setEnabled(true);
			aLeaveGame->setEnabled(true);
		
			game->queryGameState();
			
			phasesToolbar->show();
			view->show();
			break;
		}
*/		default:
			break;
	}
}

// Actions

void MainWindow::actConnect()
{
	DlgConnect dlg(this);
	if (dlg.exec())
		client->connectToServer(dlg.getHost(), dlg.getPort(), dlg.getPlayerName(), dlg.getPassword());
}

void MainWindow::actDisconnect()
{
	client->disconnectFromServer();
}

void MainWindow::actRestartGame()
{
	zoneLayout->clear();
	game->restartGameDialog();
}

void MainWindow::actLeaveGame()
{
	client->leaveGame();
}

void MainWindow::actDeckEditor()
{
	WndDeckEditor *deckEditor = new WndDeckEditor(db, this);
	deckEditor->show();
}

void MainWindow::actFullScreen(bool checked)
{
	if (checked)
		setWindowState(windowState() | Qt::WindowFullScreen);
	else
		setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void MainWindow::actSettings()
{
	DlgSettings dlg(db, translator, this);
	dlg.exec();
}

void MainWindow::actExit()
{
	close();
}

void MainWindow::actSay()
{
	if (sayEdit->text().isEmpty())
		return;
	
	client->say(sayEdit->text());
	sayEdit->clear();
}

void MainWindow::serverTimeout()
{
	QMessageBox::critical(this, tr("Error"), tr("Server timeout"));
}

void MainWindow::retranslateUi()
{
	setWindowTitle(tr("Cockatrice"));

	aConnect->setText(tr("&Connect..."));
	aDisconnect->setText(tr("&Disconnect"));
	aRestartGame->setText(tr("&Restart game..."));
	aRestartGame->setShortcut(tr("F2"));
	aLeaveGame->setText(tr("&Leave game"));
	aDeckEditor->setText(tr("&Deck editor"));
	aFullScreen->setText(tr("&Full screen"));
	aFullScreen->setShortcut(tr("Ctrl+F"));
	aSettings->setText(tr("&Settings..."));
	aExit->setText(tr("&Exit"));
	aCloseMostRecentZoneView->setText(tr("Close most recent zone view"));
	aCloseMostRecentZoneView->setShortcut(tr("Esc"));
	
	cockatriceMenu->setTitle(tr("&Cockatrice"));
	
	sayLabel->setText(tr("&Say:"));
	
	cardInfo->retranslateUi();
	chatWidget->retranslateUi();
	gameSelector->retranslateUi();
	if (game)
		game->retranslateUi();
	zoneLayout->retranslateUi();
}

void MainWindow::createActions()
{
	aConnect = new QAction(this);
	connect(aConnect, SIGNAL(triggered()), this, SLOT(actConnect()));
	aDisconnect = new QAction(this);
	aDisconnect->setEnabled(false);
	connect(aDisconnect, SIGNAL(triggered()), this, SLOT(actDisconnect()));
	aRestartGame = new QAction(this);
	aRestartGame->setEnabled(false);
	connect(aRestartGame, SIGNAL(triggered()), this, SLOT(actRestartGame()));
	aLeaveGame = new QAction(this);
	aLeaveGame->setEnabled(false);
	connect(aLeaveGame, SIGNAL(triggered()), this, SLOT(actLeaveGame()));
	aDeckEditor = new QAction(this);
	connect(aDeckEditor, SIGNAL(triggered()), this, SLOT(actDeckEditor()));
	aFullScreen = new QAction(this);
	aFullScreen->setCheckable(true);
	connect(aFullScreen, SIGNAL(toggled(bool)), this, SLOT(actFullScreen(bool)));
	aSettings = new QAction(this);
	connect(aSettings, SIGNAL(triggered()), this, SLOT(actSettings()));
	aExit = new QAction(this);
	connect(aExit, SIGNAL(triggered()), this, SLOT(actExit()));

	aCloseMostRecentZoneView = new QAction(this);
	connect(aCloseMostRecentZoneView, SIGNAL(triggered()), zoneLayout, SLOT(closeMostRecentZoneView()));
	addAction(aCloseMostRecentZoneView);
}

void MainWindow::createMenus()
{
	cockatriceMenu = menuBar()->addMenu(QString());
	cockatriceMenu->addAction(aConnect);
	cockatriceMenu->addAction(aDisconnect);
	cockatriceMenu->addSeparator();
	cockatriceMenu->addAction(aRestartGame);
	cockatriceMenu->addAction(aLeaveGame);
	cockatriceMenu->addSeparator();
	cockatriceMenu->addAction(aDeckEditor);
	cockatriceMenu->addSeparator();
	cockatriceMenu->addAction(aFullScreen);
	cockatriceMenu->addSeparator();
	cockatriceMenu->addAction(aSettings);
	cockatriceMenu->addSeparator();
	cockatriceMenu->addAction(aExit);
}

MainWindow::MainWindow(QTranslator *_translator, QWidget *parent)
	: QMainWindow(parent), game(NULL), translator(_translator)
{
	QPixmapCache::setCacheLimit(200000);

	db = new CardDatabase(this);
	
	zoneLayout = new ZoneViewLayout(db);
	scene = new GameScene(zoneLayout, this);
	view = new GameView(scene);
//	view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
	view->hide();

	cardInfo = new CardInfoWidget(db);
	messageLog = new MessageLogWidget;
	sayLabel = new QLabel;
	sayEdit = new QLineEdit;
	sayLabel->setBuddy(sayEdit);
	pingWidget = new PingWidget;
	
	client = new Client(this);
	gameSelector = new GameSelector(client);
	gameSelector->hide();
	chatWidget = new ChatWidget(client);
	chatWidget->hide();

	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->addWidget(sayLabel);
	hLayout->addWidget(sayEdit);
	hLayout->addWidget(pingWidget);

	QVBoxLayout *verticalLayout = new QVBoxLayout;
	verticalLayout->addWidget(cardInfo);
	verticalLayout->addWidget(messageLog);
	verticalLayout->addLayout(hLayout);

	viewLayout = new QVBoxLayout;
	viewLayout->addWidget(gameSelector);
	viewLayout->addWidget(chatWidget);
	viewLayout->addWidget(view);

	phasesToolbar = new PhasesToolbar;
	phasesToolbar->hide();

	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addWidget(phasesToolbar);
	mainLayout->addLayout(viewLayout, 10);
	mainLayout->addLayout(verticalLayout);

	QWidget *centralWidget = new QWidget;
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);

	connect(sayEdit, SIGNAL(returnPressed()), this, SLOT(actSay()));

	connect(client, SIGNAL(maxPingTime(int, int)), pingWidget, SLOT(setPercentage(int, int)));
	connect(client, SIGNAL(serverTimeout()), this, SLOT(serverTimeout()));
	connect(client, SIGNAL(statusChanged(ClientStatus)), this, SLOT(statusChanged(ClientStatus)));

	connect(this, SIGNAL(logConnecting(QString)), messageLog, SLOT(logConnecting(QString)));
	connect(client, SIGNAL(welcomeMsgReceived(QString)), messageLog, SLOT(logConnected(QString)));
	connect(this, SIGNAL(logDisconnected()), messageLog, SLOT(logDisconnected()));
	connect(client, SIGNAL(logSocketError(const QString &)), messageLog, SLOT(logSocketError(const QString &)));
	connect(client, SIGNAL(serverError(ServerResponse)), messageLog, SLOT(logServerError(ServerResponse)));
	connect(client, SIGNAL(protocolVersionMismatch()), messageLog, SLOT(logProtocolVersionMismatch()));
	connect(client, SIGNAL(protocolError()), messageLog, SLOT(logProtocolError()));
	connect(phasesToolbar, SIGNAL(signalSetPhase(int)), client, SLOT(setActivePhase(int)));
	connect(phasesToolbar, SIGNAL(signalNextTurn()), client, SLOT(nextTurn()));

	createActions();
	createMenus();
	
	retranslateUi();
	
	resize(900, 700);
}

void MainWindow::closeEvent(QCloseEvent */*event*/)
{
	delete game;
	chatWidget->disableChat();
	gameSelector->disableGameList();
}

void MainWindow::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
		retranslateUi();
	QMainWindow::changeEvent(event);
}
