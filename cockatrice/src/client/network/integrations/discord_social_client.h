#ifndef COCKATRICE_DISCORD_SOCIAL_CLIENT_H
#define COCKATRICE_DISCORD_SOCIAL_CLIENT_H
#include "../../../interface/widgets/tabs/tab_game.h"
#include "discordpp.h"

#include <QMap>
#include <QObject>

class DiscordSocialClient : public QObject
{
    Q_OBJECT
public:
    DiscordSocialClient(QObject *parent = nullptr);
    // Call when a game tab opens
    void joinLobbyForGame(const QString &hostname, const QString &port, int roomId, int gameId);
    // Call when a game tab closes
    void leaveGameLobby(int gameId);

    // For Rich Presence invite flow: set the joinSecret so Discord
    // can hand it back to another user who clicks "Join"
    void updateActivityWithGame(const QString &hostname,
                                const QString &port,
                                int roomId,
                                int gameId,
                                int partySize,
                                int maxPartySize);

public slots:
    void onGameTabOpened(TabGame *tab);
    void onGameTabClosed(TabGame *tab);

signals:
    void joinGameRequested(const QString &hostname, const QString &port, int roomId, int gameId);

private:
    QString makeSecret(const QString &hostname, const QString &port, int roomId, int gameId) const;

    std::shared_ptr<discordpp::Client> client;
    QMap<int, uint64_t> gameIdToLobbyId; // track open lobbies
};

#endif // COCKATRICE_DISCORD_SOCIAL_CLIENT_H
