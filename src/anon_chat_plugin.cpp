#include "anon_chat_plugin.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

AnonChatPlugin::AnonChatPlugin(QObject* parent)
    : QObject(parent)
{
    m_usernameRegistry = ffi_username_registry_new();
    m_roomRegistry = ffi_room_registry_new();
    m_moderatorRegistry = ffi_moderator_registry_new();
    m_blacklist = ffi_blacklist_new();
    qDebug() << "AnonChatPlugin: constructed and registries initialized";
}

AnonChatPlugin::~AnonChatPlugin()
{
    if (m_registration) { ffi_registration_free(m_registration); m_registration = nullptr; }
    if (m_usernameRegistry) { ffi_username_registry_free(m_usernameRegistry); m_usernameRegistry = nullptr; }
    if (m_roomRegistry) { ffi_room_registry_free(m_roomRegistry); m_roomRegistry = nullptr; }
    if (m_moderatorRegistry) { ffi_moderator_registry_free(m_moderatorRegistry); m_moderatorRegistry = nullptr; }
    if (m_blacklist) { ffi_blacklist_free(m_blacklist); m_blacklist = nullptr; }
    if (m_member) { ffi_member_free(m_member); m_member = nullptr; }
    if (m_moderator) { ffi_moderator_free(m_moderator); m_moderator = nullptr; }
    if (m_aggregator) { ffi_aggregator_free(m_aggregator); m_aggregator = nullptr; }
    qDebug() << "AnonChatPlugin: destroyed";
}

QByteArray AnonChatPlugin::hexToBytes(const QString& hex)
{
    return QByteArray::fromHex(hex.toUtf8());
}

// ---------------------------------------------------------------------------
// Identity Operations
// ---------------------------------------------------------------------------

QString AnonChatPlugin::createIdentity(const QString& nskHex)
{
    if (m_registration) {
        ffi_registration_free(m_registration);
        m_registration = nullptr;
    }

    if (nskHex.trimmed().isEmpty()) {
        m_registration = ffi_registration_new();
    } else {
        QByteArray nsk = hexToBytes(nskHex);
        if (nsk.size() != 32) return R"({"error":"NSK must be 32 bytes"})";
        m_registration = ffi_registration_from_nsk(reinterpret_cast<const uint8_t*>(nsk.constData()));
    }

    if (!m_registration) return R"({"error":"failed to create registration client"})";

    uint8_t commitment[32];
    ffi_registration_commitment(m_registration, commitment);
    QString commHex = QByteArray(reinterpret_cast<const char*>(commitment), 32).toHex();

    uint8_t nsk[32];
    ffi_registration_nsk(m_registration, nsk);
    QString secretHex = QByteArray(reinterpret_cast<const char*>(nsk), 32).toHex();

    QJsonObject obj;
    obj["ok"] = true;
    obj["commitment"] = commHex;
    obj["nsk"] = secretHex;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QString AnonChatPlugin::getCommitment()
{
    if (!m_registration) return QString();
    uint8_t commitment[32];
    ffi_registration_commitment(m_registration, commitment);
    return QByteArray(reinterpret_cast<const char*>(commitment), 32).toHex();
}

QString AnonChatPlugin::getNsk()
{
    if (!m_registration) return QString();
    uint8_t nsk[32];
    ffi_registration_nsk(m_registration, nsk);
    return QByteArray(reinterpret_cast<const char*>(nsk), 32).toHex();
}

QString AnonChatPlugin::prepareRegistration(const QString& username,
                                           const QString& nodePubkeysJson,
                                           int kThreshold)
{
    if (!m_registration) return R"({"error":"identity not initialized"})";

    QJsonDocument doc = QJsonDocument::fromJson(nodePubkeysJson.toUtf8());
    QJsonArray arr = doc.array();
    QByteArray flatKeys;
    for (const auto& val : arr) {
        QByteArray key = hexToBytes(val.toString());
        if (key.size() != 32) return R"({"error":"each node pubkey must be 32 bytes"})";
        flatKeys.append(key);
    }

    char* res = ffi_registration_prepare(
        m_registration,
        username.toUtf8().constData(),
        reinterpret_cast<const uint8_t*>(flatKeys.constData()),
        static_cast<uint32_t>(arr.size()),
        static_cast<uint32_t>(kThreshold)
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

QString AnonChatPlugin::registerUsername(const QString& commitmentHex, const QString& username)
{
    if (!m_usernameRegistry) return R"({"error":"username registry not initialized"})";
    QByteArray commBytes = hexToBytes(commitmentHex);
    if (commBytes.size() != 32) return R"({"error":"commitment must be 32 bytes"})";

    char* res = ffi_username_registry_register(
        m_usernameRegistry,
        reinterpret_cast<const uint8_t*>(commBytes.constData()),
        username.toUtf8().constData()
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

QString AnonChatPlugin::lookupUsername(const QString& commitmentHex)
{
    if (!m_usernameRegistry) return QString();
    QByteArray commBytes = hexToBytes(commitmentHex);
    if (commBytes.size() != 32) return QString();

    char* res = ffi_username_registry_lookup_by_commitment(
        m_usernameRegistry,
        reinterpret_cast<const uint8_t*>(commBytes.constData())
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

QString AnonChatPlugin::lookupCommitment(const QString& username)
{
    if (!m_usernameRegistry) return QString();
    char* res = ffi_username_registry_lookup_by_username(
        m_usernameRegistry,
        username.toUtf8().constData()
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

// ---------------------------------------------------------------------------
// Room Operations
// ---------------------------------------------------------------------------

QString AnonChatPlugin::createRoom(const QString& adminCommitmentHex,
                                   int nModThreshold,
                                   int mModTotal,
                                   const QString& modPubkeysJson,
                                   int minMembers)
{
    if (!m_roomRegistry) return R"({"error":"room registry not initialized"})";
    QByteArray adminComm = hexToBytes(adminCommitmentHex);
    if (adminComm.size() != 32) return R"({"error":"admin commitment must be 32 bytes"})";

    QJsonDocument doc = QJsonDocument::fromJson(modPubkeysJson.toUtf8());
    QJsonArray arr = doc.array();
    QByteArray flatKeys;
    for (const auto& val : arr) {
        QByteArray key = hexToBytes(val.toString());
        if (key.size() != 32) return R"({"error":"each moderator pubkey must be 32 bytes"})";
        flatKeys.append(key);
    }

    char* res = ffi_room_registry_create_room(
        m_roomRegistry,
        reinterpret_cast<const uint8_t*>(adminComm.constData()),
        static_cast<uint32_t>(nModThreshold),
        static_cast<uint32_t>(mModTotal),
        reinterpret_cast<const uint8_t*>(flatKeys.constData()),
        0, // creation index
        static_cast<uint32_t>(minMembers)
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);

    // Also register moderators in moderator registry
    if (m_moderatorRegistry) {
        char* regRes = ffi_moderator_registry_register_from_config(m_moderatorRegistry, out.toUtf8().constData());
        ffi_identity_free_string(regRes);
    }

    return out;
}

QString AnonChatPlugin::joinRoom(const QString& roomIdHex,
                                 const QString& memberCommitmentHex,
                                 const QString& memberPubkeyHex,
                                 const QString& nskHex)
{
    if (!m_roomRegistry) return R"({"error":"room registry not initialized"})";
    QByteArray roomId = hexToBytes(roomIdHex);
    QByteArray comm = hexToBytes(memberCommitmentHex);
    QByteArray pubkey = hexToBytes(memberPubkeyHex);
    QByteArray nsk = hexToBytes(nskHex);

    if (roomId.size() != 32 || comm.size() != 32 || pubkey.size() != 32 || nsk.size() != 32) {
        return R"({"error":"invalid 32-byte parameters"})";
    }

    // Sign join consent
    char* sigJson = ffi_room_sign_join_consent(
        reinterpret_cast<const uint8_t*>(roomId.constData()),
        reinterpret_cast<const uint8_t*>(comm.constData()),
        reinterpret_cast<const uint8_t*>(nsk.constData())
    );
    QByteArray sigBytes = hexToBytes(QJsonDocument::fromJson(sigJson).object()["signature"].toString());
    ffi_identity_free_string(sigJson);

    char* res = ffi_room_registry_join_room(
        m_roomRegistry,
        reinterpret_cast<const uint8_t*>(roomId.constData()),
        reinterpret_cast<const uint8_t*>(comm.constData()),
        reinterpret_cast<const uint8_t*>(pubkey.constData()),
        reinterpret_cast<const uint8_t*>(sigBytes.constData()),
        1 // join index
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

QString AnonChatPlugin::leaveRoom(const QString& roomIdHex, const QString& memberCommitmentHex)
{
    if (!m_roomRegistry) return R"({"error":"room registry not initialized"})";
    QByteArray roomId = hexToBytes(roomIdHex);
    QByteArray comm = hexToBytes(memberCommitmentHex);

    char* res = ffi_room_registry_leave_room(
        m_roomRegistry,
        reinterpret_cast<const uint8_t*>(roomId.constData()),
        reinterpret_cast<const uint8_t*>(comm.constData())
    );

    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

int AnonChatPlugin::getRoomMemberCount(const QString& roomIdHex)
{
    if (!m_roomRegistry) return 0;
    QByteArray roomId = hexToBytes(roomIdHex);
    return ffi_room_registry_active_member_count(m_roomRegistry, reinterpret_cast<const uint8_t*>(roomId.constData()));
}

bool AnonChatPlugin::isRoomMember(const QString& roomIdHex, const QString& memberCommitmentHex)
{
    if (!m_roomRegistry) return false;
    QByteArray roomId = hexToBytes(roomIdHex);
    QByteArray comm = hexToBytes(memberCommitmentHex);
    return ffi_room_registry_has_active_membership(m_roomRegistry,
                                                   reinterpret_cast<const uint8_t*>(roomId.constData()),
                                                   reinterpret_cast<const uint8_t*>(comm.constData())) == 1;
}

// ---------------------------------------------------------------------------
// Chat & Messaging Operations
// ---------------------------------------------------------------------------

QString AnonChatPlugin::preparePost(const QString& message,
                                    const QString& postSaltHex,
                                    const QString& moderatorPubkeysJson,
                                    int nThreshold)
{
    if (!m_registration) return R"({"error":"identity not initialized"})";

    uint8_t nsk[32];
    ffi_registration_nsk(m_registration, nsk);

    // Instantiate or reuse MemberClient
    if (!m_member) {
        m_member = ffi_member_new(nsk, 3); // default k_strikes = 3
    }

    QByteArray msgBytes = message.toUtf8();
    QByteArray salt = hexToBytes(postSaltHex);
    if (salt.size() != 32) return R"({"error":"post salt must be 32 bytes"})";

    QJsonDocument doc = QJsonDocument::fromJson(moderatorPubkeysJson.toUtf8());
    QJsonArray arr = doc.array();
    QByteArray flatKeys;
    for (const auto& val : arr) {
        QByteArray key = hexToBytes(val.toString());
        if (key.size() != 32) return R"({"error":"each pubkey must be 32 bytes"})";
        flatKeys.append(key);
    }

    char* res = ffi_member_prepare_post(
        m_member,
        reinterpret_cast<const uint8_t*>(msgBytes.constData()),
        static_cast<uint32_t>(msgBytes.size()),
        reinterpret_cast<const uint8_t*>(salt.constData()),
        reinterpret_cast<const uint8_t*>(flatKeys.constData()),
        static_cast<uint32_t>(arr.size()),
        static_cast<uint32_t>(nThreshold)
    );

    QString out = QString::fromUtf8(res);
    ffi_free_string(res);
    return out;
}

// ---------------------------------------------------------------------------
// Moderation & Strike Operations
// ---------------------------------------------------------------------------

QString AnonChatPlugin::createModerator(const QString& privkeyHex)
{
    if (m_moderator) { ffi_moderator_free(m_moderator); m_moderator = nullptr; }
    QByteArray priv = hexToBytes(privkeyHex);
    if (priv.size() != 32) return R"({"error":"privkey must be 32 bytes"})";

    m_moderator = ffi_moderator_new(reinterpret_cast<const uint8_t*>(priv.constData()));
    if (!m_moderator) return R"({"error":"failed to create moderator"})";
    return R"({"ok":true})";
}

QString AnonChatPlugin::getModeratorPubkey()
{
    if (!m_moderator) return QString();
    uint8_t pub[32];
    ffi_moderator_public_key(m_moderator, pub);
    return QByteArray(reinterpret_cast<const char*>(pub), 32).toHex();
}

QString AnonChatPlugin::issueStrike(const QString& roomIdHex,
                                    const QString& targetCommitmentHex,
                                    const QString& evidenceHashHex,
                                    const QString& tracingTagHex,
                                    const QString& encryptedShareJson,
                                    int moderatorIndex)
{
    if (!m_moderator) return R"({"error":"moderator not initialized"})";
    QByteArray tag = hexToBytes(tracingTagHex);
    if (tag.size() != 32) return R"({"error":"tracing tag must be 32 bytes"})";

    char* res = ffi_moderator_issue_strike(
        m_moderator,
        reinterpret_cast<const uint8_t*>(tag.constData()),
        encryptedShareJson.toUtf8().constData(),
        static_cast<uint32_t>(moderatorIndex)
    );

    QString out = QString::fromUtf8(res);
    ffi_free_string(res);
    return out;
}

QString AnonChatPlugin::validateStrike(const QString& certificateJson, int nThreshold)
{
    if (!m_moderatorRegistry) return R"({"error":"moderator registry not initialized"})";
    char* res = ffi_strike_validate(certificateJson.toUtf8().constData(), static_cast<uint32_t>(nThreshold), m_moderatorRegistry);
    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}

// ---------------------------------------------------------------------------
// Slashing & Reconstruction Operations
// ---------------------------------------------------------------------------

QString AnonChatPlugin::createAggregator(int nThreshold, int kStrikes, const QString& moderatorPubkeysJson)
{
    if (m_aggregator) { ffi_aggregator_free(m_aggregator); m_aggregator = nullptr; }
    QJsonDocument doc = QJsonDocument::fromJson(moderatorPubkeysJson.toUtf8());
    QJsonArray arr = doc.array();
    QByteArray flatKeys;
    for (const auto& val : arr) {
        flatKeys.append(hexToBytes(val.toString()));
    }

    m_aggregator = ffi_aggregator_new(
        static_cast<uint32_t>(nThreshold),
        static_cast<uint32_t>(kStrikes),
        reinterpret_cast<const uint8_t*>(flatKeys.constData()),
        static_cast<uint32_t>(arr.size())
    );

    if (!m_aggregator) return R"({"error":"failed to create aggregator"})";
    return R"({"ok":true})";
}

QString AnonChatPlugin::reconstructStrike(const QString& tracingTagHex, const QString& certificatesJson)
{
    if (!m_aggregator) return R"({"error":"aggregator not initialized"})";
    QByteArray tag = hexToBytes(tracingTagHex);
    char* res = ffi_aggregator_reconstruct_strike(
        m_aggregator,
        reinterpret_cast<const uint8_t*>(tag.constData()),
        certificatesJson.toUtf8().constData()
    );
    QString out = QString::fromUtf8(res);
    ffi_free_string(res);
    return out;
}

QString AnonChatPlugin::reconstructNsk(const QString& strikesJson)
{
    if (!m_aggregator) return R"({"error":"aggregator not initialized"})";
    char* res = ffi_aggregator_reconstruct_nsk(m_aggregator, strikesJson.toUtf8().constData());
    QString out = QString::fromUtf8(res);
    ffi_free_string(res);
    return out;
}

bool AnonChatPlugin::isRevoked(const QString& commitmentHex)
{
    if (!m_blacklist) return false;
    QByteArray comm = hexToBytes(commitmentHex);
    return ffi_blacklist_is_revoked(m_blacklist, reinterpret_cast<const uint8_t*>(comm.constData())) == 1;
}

QString AnonChatPlugin::revokeCommitment(const QString& commitmentHex)
{
    if (!m_blacklist) return R"({"error":"blacklist not initialized"})";
    QByteArray comm = hexToBytes(commitmentHex);
    char* res = ffi_blacklist_revoke(m_blacklist, reinterpret_cast<const uint8_t*>(comm.constData()));
    QString out = QString::fromUtf8(res);
    ffi_identity_free_string(res);
    return out;
}
