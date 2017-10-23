#include "CFrameSyncGameModule.h"
#include "NFComm/NFMessageDefine/NFMsgDefine.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFIKernelModule.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFPluginModule/NFISceneProcessModule.h"
#include "NFComm/NFPluginModule/NFIElementModule.h"
#include "NFComm/NFPluginModule/NFIEventModule.h"
#include "NFComm/NFPluginModule/NFISceneAOIModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToWorldModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFIPlayerRedisModule.h"

//EGMI_REQ_BATTLE_MATCH = 2000; //?????Game ??????
//EGMI_ACK_BATTLE_MATCH = 2001; //Game????? ?????????????????????????
//EGMI_REQ_BATTLE_START = 2002; //?????Game????
//EGMI_ACK_BATTLE_START = 2003; //Game????? ?????????????
//EGMI_NTF_CG_BATTLE_FRAMECOMMAND = 2000; //?????????????????
//EGMI_NTF_GC_BATTLE_FRAMECOMMAND = 2000; //?????????????
//EGMI_NTF_GC_BATTLE_FRAMEFINISH = 2000; //??????????????????????????


CFrameSyncGameModule::CFrameSyncGameModule(NFIPluginManager* p)
{
	pPluginManager = p;
}


CFrameSyncGameModule::~CFrameSyncGameModule()
{
}

bool CFrameSyncGameModule::Init()
{
	m_pNetModule = pPluginManager->FindModule<NFINetModule>();
	return true;
}

bool CFrameSyncGameModule::AfterInit()
{
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_REQ_BATTLE_MATCH, this, &CFrameSyncGameModule::OnReqBattleMatchProcess);
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_NTF_CG_BATTLE_FRAMECOMMAND, this, &CFrameSyncGameModule::OnNtfCGBattleFrameCommandProcess);

	
	return true;
}
bool CFrameSyncGameModule::ReadyExecute()
{
	miLastTime = NFGetTime();
	return true;
}
bool CFrameSyncGameModule::Execute()
{
	ProcessMatch();
	ProcessFrameSync();

	return true;
}

void CFrameSyncGameModule::OnReqBattleMatchProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGUID nClientID;
	NFMsg::ReqBattleMatch xMsg;
	if (!m_pNetModule->ReceivePB(nMsgID, msg, nLen, xMsg, nClientID))
	{
		return;
	}

	BattleMatchPlayerInfo pi;
	pi.ClientID = nClientID;
	pi.ProxySocketIndex = nSockIndex;
	mMatchingPlayers.push_back(pi);
}

void CFrameSyncGameModule::ProcessMatchSuccess(const BattleInfo & bi)
{
	NFMsg::AckBattleMatch ret;
	NFMsg::PlayerInfo& pi_a = *ret.mutable_playera();

}

void CFrameSyncGameModule::SendFrameFinishCmmand(const BattleInfo& bi)
{
}
//EGMI_NTF_CG_BATTLE_FRAMECOMMAND
void CFrameSyncGameModule::OnNtfCGBattleFrameCommandProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{



}

 void CFrameSyncGameModule::ProcessMatch() 
 {
	 while (mMatchingPlayers.size() > 1)
	 {
		 BattleInfo bi;
		 bi.playerA = mMatchingPlayers[0];
		 bi.playerB = mMatchingPlayers[1];
		 mMatchingPlayers.erase(mMatchingPlayers.begin());
		 mMatchingPlayers.erase(mMatchingPlayers.begin());
		 ProcessMatchSuccess(bi);
	 }
 }

 void CFrameSyncGameModule::ProcessFrameSync()
 {
	 uint64_t timeNow = NFGetTime();

	 if (timeNow - miLastTime < FRAMESYNC_TIMESPAN)
		 return;

	 //process frame command EGMI_NTF_GC_BATTLE_FRAMEFINISH
	 RuningBattleIterator begin = mRuningBattle.cbegin();
	 RuningBattleIterator end = mRuningBattle.cend();

	 for (; begin != end; ++begin)
	 {
		 const BattleInfo& bi = begin->second;
		 SendFrameFinishCmmand(bi);
	 }
 }
