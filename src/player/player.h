#include "common.h"
#include "steam/isteamuser.h"
#include "sdk/serversideclient.h"
#include "sdk/datatypes.h"
#include "sdk/services.h"
#include "sdk/entity/ccsplayercontroller.h"
#include "utils/utils.h"

class ns_address;
class C2S_CONNECT_Message;

// The index is the entity index associated with the player's controller's entity index.
// The player slot should always be this index minus 1.
class Player
{
public:
	Player(int i) : index(i) {}

	virtual ~Player() {}

	virtual void Init() {}

	virtual void Reset()
	{
		unauthenticatedSteamID = k_steamIDNil;
	}

	virtual CCSPlayerController *GetController();

	virtual CBasePlayerPawn *GetCurrentPawn()
	{
		return GetController() ? GetController()->GetCurrentPawn() : nullptr;
	};

	virtual CCSPlayerPawn *GetPlayerPawn()
	{
		return GetController() ? GetController()->GetPlayerPawn() : nullptr;
	}

	virtual CCSPlayerPawnBase *GetObserverPawn()
	{
		return GetController() ? GetController()->GetObserverPawn() : nullptr;
	}

	CPlayerSlot GetPlayerSlot()
	{
		return index - 1;
	}

	bool IsAuthenticated()
	{
		return IsConnected() && !IsFakeClient() && interfaces::pEngine->IsClientFullyAuthenticated(GetPlayerSlot());
	}

	bool IsConnected()
	{
		return GetClient() ? GetClient()->IsConnected() : false;
	}

	bool IsInGame()
	{
		return GetClient() ? GetClient()->IsInGame() : false;
	}

	bool IsFakeClient()
	{
		return GetClient() ? GetClient()->IsFakePlayer() : false;
	}

	bool IsCSTV()
	{
		return GetClient() ? GetClient()->IsHLTV() : false;
	};

	void Kick(const char *internalReason = "", ENetworkDisconnectionReason reason = NETWORK_DISCONNECT_KICKED);

	const char *GetName()
	{
		return GetClient() ? GetClient()->GetClientName() : "<blank>";
	}

	const char *GetIpAddress()
	{
		return GetClient() ? GetClient()->GetRemoteAddress()->ToString(true) : nullptr;
	}

	const CSteamID &GetSteamId(bool validated = true)
	{
		static_persist const CSteamID invalidId = k_steamIDNil;
		if (validated && !IsAuthenticated())
		{
			return invalidId;
		}
		CServerSideClient *client = GetClient();
		if (client && IsAuthenticated())
		{
			return *client->GetClientSteamID();
		}
		return unauthenticatedSteamID;
	}

	u64 GetSteamId64(bool validated = true)
	{
		return GetSteamId(validated).ConvertToUint64();
	}

	CServerSideClient *GetClient()
	{
		return g_pKZUtils->GetClientBySlot(GetPlayerSlot());
	}

	void SetUnauthenticatedSteamID(u64 xuid)
	{
		unauthenticatedSteamID = CSteamID(xuid);
	}

	virtual void OnPlayerConnect(u64 steamID64) {}

	virtual void OnPlayerActive() {}

	virtual void OnPlayerFullyConnect() {}

	virtual void OnAuthorized();

public:
	// General
	const i32 index;
	bool hasPrime {};

private:
	CSteamID unauthenticatedSteamID = k_steamIDNil;
};

class PlayerManager
{
public:
	PlayerManager()
	{
		for (int i = 0; i < MAXPLAYERS + 1; i++)
		{
			delete players[i];
			players[i] = new Player(i);
			players[i]->Reset();
		}
	}

	virtual ~PlayerManager()
	{
		for (int i = 0; i < MAXPLAYERS + 1; i++)
		{
			delete players[i];
		}
	}

public:
	Player *ToPlayer(CPlayerPawnComponent *component);
	Player *ToPlayer(CBasePlayerController *controller);
	Player *ToPlayer(CBasePlayerPawn *pawn);
	Player *ToPlayer(CPlayerSlot slot);
	Player *ToPlayer(CEntityIndex entIndex);
	Player *ToPlayer(CPlayerUserId userID);
	Player *SteamIdToPlayer(u64 steamID, bool validated = true);

	virtual void ResetPlayers()
	{
		for (int i = 0; i < MAXPLAYERS + 1; i++)
		{
			players[i]->Reset();
		}
	}

	void Cleanup();
	void OnLateLoad();

	void OnSteamAPIActivated();

	/*
		The connect events actually happen in this order.
		Only these functions are called for bots: OnClientConnected, OnClientPutInServer, OnClientActive, OnClientDisconnect
	*/

	// These following functions are NOT called upon map change.
	// Note: ConnectClient can be triggered multiple times with MultiAddonsManager.
	void OnConnectClient(const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, const char *pszChallenge,
						 const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	void OnClientConnected(CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer);
	void OnConnectClientPost(const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, const char *pszChallenge,
							 const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);

	// These following functions are called upon map change.
	void OnClientConnect(CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason);
	void OnClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid);
	void OnClientActive(CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid);
	void OnClientFullyConnect(CPlayerSlot slot);
	void OnClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID);

	void OnClientVoice(CPlayerSlot slot);
	void OnClientSettingsChanged(CPlayerSlot slot);

	STEAM_GAMESERVER_CALLBACK_MANUAL(PlayerManager, OnValidateAuthTicket, ValidateAuthTicketResponse_t, m_CallbackValidateAuthTicketResponse);

	void RegisterSteamAPICallback()
	{
		if (callbackRegistered)
		{
			return;
		}
		callbackRegistered = true;
		m_CallbackValidateAuthTicketResponse.Register(this, &PlayerManager::OnValidateAuthTicket);
	}

private:
	bool callbackRegistered {};

public:
	Player *players[MAXPLAYERS + 1];
};

extern PlayerManager *g_pPlayerManager;
