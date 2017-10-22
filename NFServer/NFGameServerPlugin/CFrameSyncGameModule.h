#ifndef CFRAMESYNC_GAME_MODULE_H
#define CFRAMESYNC_GAME_MODULE_H

#include "IFrameSyncGameModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"

struct BattleMatchPlayerInfo
{
	NFGUID ClientID;
};

struct BattleInfo
{
	BattleMatchPlayerInfo playerA;
	BattleMatchPlayerInfo playerB;
};

class CFrameSyncGameModule :
	public IFrameSyncGameModule
{
public:
	CFrameSyncGameModule(NFIPluginManager* p);
	virtual ~CFrameSyncGameModule();


	virtual bool Init();
	virtual bool AfterInit();


	//net msg process
private:
	void OnReqBattleMatchProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

private:
	std::map<NFGUID, struct BattleInfo> mRuningBattle;
	std::vector<struct BattleMatchPlayerInfo> mMatchingPlayers;
	NFINetModule* m_pNetModule;
};

#endif 