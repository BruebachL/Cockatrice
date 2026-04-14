#include "discord_social_client.h"

#define DISCORDPP_IMPLEMENTATION
#include "../../../interface/widgets/tabs/tab_game.h"
#include "discordpp.h"
#include "remote_client.h"

#include <QDebug>
#include <QTimer>

const uint64_t APPLICATION_ID = 1489873419875913832;

DiscordSocialClient::DiscordSocialClient(QObject *parent)
{
    Q_UNUSED(parent);

    client = std::make_shared<discordpp::Client>();
    client->AddLogCallback(
        [](auto message, auto severity) { qInfo() << "[" << EnumToString(severity) << "] " << message; },
        discordpp::LoggingSeverity::Info);

    client->RegisterLaunchCommand(APPLICATION_ID, "cockatrice://");

    client->SetStatusChangedCallback(
        [this](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
            qInfo() << "🔄 Status changed: " << discordpp::Client::StatusToString(status);

            if (status == discordpp::Client::Status::Ready) {
                qInfo() << "✅ Client is ready! You can now call SDK functions.\n";
                qInfo() << "👥 Friends Count: " << client->GetRelationships().size();
                // Configure rich presence details
                discordpp::Activity activity;
                activity.SetType(discordpp::ActivityTypes::Playing);
                activity.SetState("Coding features");
                activity.SetDetails("Working on integrating Discord");

                // Update rich presence
                client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
                    if (result.Successful()) {
                        qInfo() << "🎮 Rich Presence updated successfully!\n";
                    } else {
                        qInfo() << "❌ Rich Presence update failed";
                    }
                });
            } else if (error != discordpp::Client::Error::None) {
                qInfo() << "❌ Connection Error: " << discordpp::Client::ErrorToString(error)
                        << " - Details: " << errorDetail;
            }
        });

    // Generate OAuth2 code verifier for authentication
    auto codeVerifier = client->CreateAuthorizationCodeVerifier();

    // Set up authentication arguments
    discordpp::AuthorizationArgs args{};
    args.SetClientId(APPLICATION_ID);
    args.SetScopes(discordpp::Client::GetDefaultCommunicationScopes());
    args.SetCodeChallenge(codeVerifier.Challenge());

    // Begin authentication process
    client->Authorize(args, [this, codeVerifier](auto result, auto code, auto redirectUri) {
        if (!result.Successful()) {
            qInfo() << "❌ Authentication Error: " << result.Error();
            return;
        } else {
            qInfo() << "✅ Authorization successful! Getting access token...\n";

            // Exchange auth code for access token
            client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
                             [this](discordpp::ClientResult result, std::string accessToken, std::string refreshToken,
                                    discordpp::AuthorizationTokenType tokenType, int32_t expiresIn, std::string scope) {
                                 Q_UNUSED(result);
                                 Q_UNUSED(expiresIn);
                                 Q_UNUSED(refreshToken);
                                 Q_UNUSED(tokenType);
                                 Q_UNUSED(scope);
                                 qInfo() << "🔓 Access token received! Establishing connection...\n";
                                 client->UpdateToken(discordpp::AuthorizationTokenType::Bearer, accessToken,
                                                     [this](discordpp::ClientResult result) {
                                                         if (result.Successful()) {
                                                             qInfo() << "🔑 Token updated, connecting to Discord...\n";
                                                             client->Connect();
                                                         }
                                                     });
                             });
        }
    });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, []() { discordpp::RunCallbacks(); });

    timer->start(10);
}

QString DiscordSocialClient::makeSecret(const QString &hostname, const QString &port, int roomId, int gameId) const
{
    return QString("%1:%2:%3:%4").arg(hostname, port).arg(roomId).arg(gameId);
}

void DiscordSocialClient::joinLobbyForGame(const QString &hostname, const QString &port, int roomId, int gameId)
{
    if (!client)
        return;
    QString secret = makeSecret(hostname, port, roomId, gameId);

    client->CreateOrJoinLobby(secret.toStdString(), [this, gameId](discordpp::ClientResult result, uint64_t lobbyId) {
        if (result.Successful()) {
            qInfo() << "🎮 Joined Discord lobby" << lobbyId << "for game" << gameId;
            gameIdToLobbyId.insert(gameId, lobbyId);
        } else {
            qWarning() << "❌ Failed to join Discord lobby for game" << gameId;
        }
    });
}

void DiscordSocialClient::leaveGameLobby(int gameId)
{
    if (!client || !gameIdToLobbyId.contains(gameId))
        return;
    uint64_t lobbyId = gameIdToLobbyId.take(gameId);

    client->LeaveLobby(lobbyId, [lobbyId](discordpp::ClientResult result) {
        if (result.Successful())
            qInfo() << "👋 Left Discord lobby" << lobbyId;
    });
}

void DiscordSocialClient::updateActivityWithGame(const QString &hostname,
                                                 const QString &port,
                                                 int roomId,
                                                 int gameId,
                                                 int partySize,
                                                 int maxPartySize)
{
    QString joinUrl = QString("cockatrice://joinGame?hostname=%1&port=%2&roomid=%3&gameid=%4")
                          .arg(hostname, port)
                          .arg(roomId)
                          .arg(gameId);

    discordpp::ActivitySecrets secrets;
    secrets.SetJoin(joinUrl.toStdString());

    discordpp::ActivityParty party;
    party.SetId(QString::number(gameId).toStdString());
    party.SetCurrentSize(partySize);
    party.SetMaxSize(maxPartySize);

    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetDetails("In a game");
    activity.SetSecrets(secrets);
    activity.SetParty(party);

    client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
        if (!result.Successful()) {
            qWarning() << "❌ Rich Presence update failed:" << result.Error();
        } else {
            qInfo() << "✅ Rich Presence updated";
        }
    });
}

void DiscordSocialClient::onGameTabOpened(TabGame *tab)
{
    // TabGame has everything we need
    QList<AbstractClient *> clients = tab->getGame()->getGameState()->getClients();
    QString hostname = static_cast<RemoteClient *>(clients.first())->peerName();
    QString port = QString::number(static_cast<RemoteClient *>(clients.first())->peerPort());
    int roomId = tab->getGame()->getGameMetaInfo()->roomId();
    int gameId = tab->getGame()->getGameMetaInfo()->gameId();
    int playerSize = tab->getGame()->getGameMetaInfo()->maxPlayers();

    joinLobbyForGame(hostname, port, roomId, gameId);
    updateActivityWithGame(hostname, port, roomId, gameId, 1, playerSize);
}

void DiscordSocialClient::onGameTabClosed(TabGame *tab)
{
    leaveGameLobby(tab->getGame()->getGameMetaInfo()->gameId());
}