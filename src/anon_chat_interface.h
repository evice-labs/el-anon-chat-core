#ifndef ANON_CHAT_INTERFACE_H
#define ANON_CHAT_INTERFACE_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include "interface.h"

class AnonChatInterface : public PluginInterface
{
public:
    virtual ~AnonChatInterface() = default;

    // --- Identity Operations ---
    Q_INVOKABLE virtual QString createIdentity(const QString& nskHex = QString()) = 0;
    Q_INVOKABLE virtual QString getCommitment() = 0;
    Q_INVOKABLE virtual QString getNsk() = 0;
    Q_INVOKABLE virtual QString prepareRegistration(const QString& username,
                                                    const QString& nodePubkeysJson,
                                                    int kThreshold) = 0;
    Q_INVOKABLE virtual QString registerUsername(const QString& commitmentHex, const QString& username) = 0;
    Q_INVOKABLE virtual QString lookupUsername(const QString& commitmentHex) = 0;
    Q_INVOKABLE virtual QString lookupCommitment(const QString& username) = 0;

    // --- Room Operations ---
    Q_INVOKABLE virtual QString createRoom(const QString& adminCommitmentHex,
                                           int nModThreshold,
                                           int mModTotal,
                                           const QString& modPubkeysJson,
                                           int minMembers) = 0;
    Q_INVOKABLE virtual QString joinRoom(const QString& roomIdHex,
                                         const QString& memberCommitmentHex,
                                         const QString& memberPubkeyHex,
                                         const QString& nskHex) = 0;
    Q_INVOKABLE virtual QString leaveRoom(const QString& roomIdHex,
                                          const QString& memberCommitmentHex) = 0;
    Q_INVOKABLE virtual int getRoomMemberCount(const QString& roomIdHex) = 0;
    Q_INVOKABLE virtual bool isRoomMember(const QString& roomIdHex,
                                          const QString& memberCommitmentHex) = 0;

    // --- Chat & Anonymous Messaging Operations ---
    Q_INVOKABLE virtual QString preparePost(const QString& message,
                                            const QString& postSaltHex,
                                            const QString& moderatorPubkeysJson,
                                            int nThreshold) = 0;

    // --- Moderation & Strike Operations ---
    Q_INVOKABLE virtual QString createModerator(const QString& privkeyHex) = 0;
    Q_INVOKABLE virtual QString getModeratorPubkey() = 0;
    Q_INVOKABLE virtual QString issueStrike(const QString& roomIdHex,
                                            const QString& targetCommitmentHex,
                                            const QString& evidenceHashHex,
                                            const QString& tracingTagHex,
                                            const QString& encryptedShareJson,
                                            int moderatorIndex) = 0;
    Q_INVOKABLE virtual QString validateStrike(const QString& certificateJson, int nThreshold) = 0;

    // --- Slashing & Blacklist Operations ---
    Q_INVOKABLE virtual QString createAggregator(int nThreshold, int kStrikes,
                                                 const QString& moderatorPubkeysJson) = 0;
    Q_INVOKABLE virtual QString reconstructStrike(const QString& tracingTagHex,
                                                  const QString& certificatesJson) = 0;
    Q_INVOKABLE virtual QString reconstructNsk(const QString& strikesJson) = 0;
    Q_INVOKABLE virtual bool isRevoked(const QString& commitmentHex) = 0;
    Q_INVOKABLE virtual QString revokeCommitment(const QString& commitmentHex) = 0;
};

#define AnonChatInterface_iid "org.logos.AnonChatInterface"
Q_DECLARE_INTERFACE(AnonChatInterface, AnonChatInterface_iid)

#endif // ANON_CHAT_INTERFACE_H
