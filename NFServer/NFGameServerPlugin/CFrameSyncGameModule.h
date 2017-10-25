#ifndef CFRAMESYNC_GAME_MODULE_H
#define CFRAMESYNC_GAME_MODULE_H

#include "IFrameSyncGameModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include <set>

class CFrameSyncGameModule :
	public IFrameSyncGameModule
{
public:
	struct BattleMatchPlayerInfo
	{
		inline BattleMatchPlayerInfo()
		{
			IsReady = false;
		}
		NFGUID RoleID;
		NFSOCK ProxySocketIndex;
		bool   IsReady;
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
	void ProcessReadyStart(NFGUID RoleID);
	void ProcessFrameSync();
	//net msg process
private:
	void OnReqBattleMatchProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnReqBattleStartProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnNtfCGBattleFrameCommandProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void ProcessMatchSuccess(NF_SHARE_PTR<BattleInfo> pBI);

	void SendFrameFinishCmmand(const NF_SHARE_PTR<BattleInfo> bi);
private:
	std::map<NFGUID, NF_SHARE_PTR<struct BattleInfo>> mRoleID2BattleInfo;
	std::set<NF_SHARE_PTR<struct BattleInfo>> mRuningBattles;
	std::map<NFGUID, NF_SHARE_PTR<struct BattleInfo>> mLoadingBattles;
	typedef std::set<NF_SHARE_PTR<struct BattleInfo>>::const_iterator RuningBattleIterator;
	std::vector<struct BattleMatchPlayerInfo> mMatchingPlayers;
	NFINetModule* m_pNetModule;
	//NFILogModule* m_pLogModule;
	NFIGameServerNet_ServerModule* m_pGameServerNet_ServerModule;
	int64_t miLastTime;
	int64_t miNowTime;

	const int64_t FRAMESYNC_TIMESPAN = 1000 / 15;//66.666
};

#endif 