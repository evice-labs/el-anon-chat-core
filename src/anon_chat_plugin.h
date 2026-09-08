#ifndef ANON_CHAT_PLUGIN_H
#define ANON_CHAT_PLUGIN_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include "anon_chat_interface.h"

extern "C" {
#include "lib/e_identity_sdk.h"
#include "lib/e_moderation_sdk.h"
}

class LogosAPI;

class AnonChatPlugin : public QObject, public AnonChatInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AnonChatInterface_iid FILE "metadata.json")
    Q_INTERFACES(AnonChatInterface PluginInterface)

public:
    explicit AnonChatPlugin(QObject* parent = nullptr);
    ~AnonChatPlugin() override;

    // PluginInterface
    QString name() const override { return "anon_chat_core"; }
    QString version() const override { return "1.0.0"; }

    // Injected by Basecamp runtime
    Q_INVOKABLE void initLogos(LogosAPI* api) { logosAPI = api; }

    // Identity Operations
    Q_INVOKABLE QString createIdentity(const QString& nskHex = QString()) override;
    Q_INVOKABLE QString getCommitment() override;
    Q_INVOKABLE QString getNsk() override;
    Q_INVOKABLE QString prepareRegistration(const QString& username,
                                            const QString& nodePubkeysJson,
                                            int kThreshold) override;
    Q_INVOKABLE QString registerUsername(const QString& commitmentHex, const QString& username) override;
    Q_INVOKABLE QString lookupUsername(const QString& commitmentHex) override;
    Q_INVOKABLE QString lookupCommitment(const QString& username) override;

    // Room Operations
    Q_INVOKABLE QString createRoom(const QString& adminCommitmentHex,
                                   int nModThreshold,
                                   int mModTotal,
                                   const QString& modPubkeysJson,
                                   int minMembers) override;
    Q_INVOKABLE QString joinRoom(const QString& roomIdHex,
                                 const QString& memberCommitmentHex,
                                 const QString& memberPubkeyHex,
                                 const QString& nskHex) override;
    Q_INVOKABLE QString leaveRoom(const QString& roomIdHex,
                                  const QString& memberCommitmentHex) override;
    Q_INVOKABLE int getRoomMemberCount(const QString& roomIdHex) override;
    Q_INVOKABLE bool isRoomMember(const QString& roomIdHex,
                                  const QString& memberCommitmentHex) override;

    // Chat & Messaging
    Q_INVOKABLE QString preparePost(const QString& message,
                                    const QString& postSaltHex,
                                    const QString& moderatorPubkeysJson,
                                    int nThreshold) override;

    // Moderation & Strikes
    Q_INVOKABLE QString createModerator(const QString& privkeyHex) override;
    Q_INVOKABLE QString getModeratorPubkey() override;
    Q_INVOKABLE QString issueStrike(const QString& roomIdHex,
                                    const QString& targetCommitmentHex,
                                    const QString& evidenceHashHex,
                                    const QString& tracingTagHex,
                                    const QString& encryptedShareJson,
                                    int moderatorIndex) override;
    Q_INVOKABLE QString validateStrike(const QString& certificateJson, int nThreshold) override;

    // Slashing & Blacklist
    Q_INVOKABLE QString createAggregator(int nThreshold, int kStrikes,
                                         const QString& moderatorPubkeysJson) override;
    Q_INVOKABLE QString reconstructStrike(const QString& tracingTagHex,
                                          const QString& certificatesJson) override;
    Q_INVOKABLE QString reconstructNsk(const QString& strikesJson) override;
    Q_INVOKABLE bool isRevoked(const QString& commitmentHex) override;
    Q_INVOKABLE virtual QString revokeCommitment(const QString& commitmentHex) override;

signals:
    void eventResponse(const QString& eventName, const QVariantList& args);

private:
    LogosAPI* logosAPI = nullptr;

    // FFI Handles
    FfiRegistrationClient* m_registration = nullptr;
    FfiUsernameRegistry* m_usernameRegistry = nullptr;
    FfiRoomRegistry* m_roomRegistry = nullptr;
    FfiModeratorRegistry* m_moderatorRegistry = nullptr;
    FfiBlacklist* m_blacklist = nullptr;
    FfiMemberClient* m_member = nullptr;
    FfiModeratorClient* m_moderator = nullptr;
    FfiSlashAggregator* m_aggregator = nullptr;

    static QByteArray hexToBytes(const QString& hex);
};

#endif // ANON_CHAT_PLUGIN_H
