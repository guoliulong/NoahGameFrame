#ifndef CFRAMESYNC_GAME_MODULE_H
#define CFRAMESYNC_GAME_MODULE_H

#include "IFrameSyncGameModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"


class CFrameSyncGameModule :
	public IFrameSyncGameModule
{
public:
	struct BattleMatchPlayerInfo
	{
		NFGUID RoleID;
		NFSOCK ProxySocketIndex;
	};

	struct BattleInfo
	{
		BattleMatchPlayerInfo playerA;
		BattleMatchPlayerInfo playerB;
	};

public:
	CFrameSyncGameModule(NFIPluginManager* p);
	virtual ~CFrameSyncGameModule();


	virtual bool Init();
	virtual bool AfterInit();
	virtual bool ReadyExecute();
	virtual bool Execute();
private:
	void ProcessMatch();
	void ProcessFrameSync();
	//net msg process
private:
	void OnReqBattleMatchProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnNtfCGBattleFrameCommandProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void ProcessMatchSuccess(const BattleInfo&bi);

	void SendFrameFinishCmmand(const BattleInfo& bi);
private:
	std::map<NFGUID, struct BattleInfo> mRuningBattle;
	typedef std::map<NFGUID, struct BattleInfo>::const_iterator RuningBattleIterator;
	std::vector<struct BattleMatchPlayerInfo> mMatchingPlayers;
	NFINetModule* m_pNetModule;
	//NFILogModule* m_pLogModule;
	NFIGameServerNet_ServerModule* m_pGameServerNet_ServerModule;
	int64_t miLastTime;
	int64_t miNowTime;

	const uint64_t FRAMESYNC_TIMESPAN = 1000 / 15;//66.666
};

#endif 