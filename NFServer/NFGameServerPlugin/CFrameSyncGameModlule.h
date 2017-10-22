#pragma once
#include "IFrameSyncGameModlule.h"



struct BattleMatchPlayerInfo
{
	NFGUID ClientID;
};

struct BattleInfo
{
	BattleMatchPlayerInfo playerA;
	BattleMatchPlayerInfo playerB;
};

class CFrameSyncGameModlule :
	public IFrameSyncGameModlule
{
public:
	CFrameSyncGameModlule();
	virtual ~CFrameSyncGameModlule();


	virtual bool Init();
	virtual bool AfterInit();


	//net msg process
private:
	void OnReqBattleMatchProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

private:
	std::map<NFGUID, BattleInfo> mRuningBattle;
	std::vector<BattleMatchPlayerInfo> mMatchingPlayers;

	NFINetModule* m_pNetModule;
};
