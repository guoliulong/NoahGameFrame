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
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
	//m_pLogModule = pPluginManager->FindModule<NFILogModule>();
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
	pi.RoleID = NFINetModule::PBToNF(xMsg.role_id());
	pi.ProxySocketIndex = nSockIndex;
	mMatchingPlayers.push_back(pi);


}

void CFrameSyncGameModule::ProcessMatchSuccess(const BattleInfo & bi)
{
	NFMsg::AckBattleMatch ret;
	ret.set_issuccessed(true);
	NFMsg::PlayerInfo& pi_a = *ret.mutable_playera();
	*pi_a.mutable_role_id() = NFINetModule::NFToPB(bi.playerA.RoleID);
	*pi_a.mutable_player_name() = "playerA";
	NFMsg::PlayerInfo& pi_b = *ret.mutable_playerb();
	*pi_b.mutable_role_id() = NFINetModule::NFToPB(bi.playerB.RoleID);
	*pi_b.mutable_player_name() = "playerB";
	//set heros
	(*pi_a.add_hero_list()).set_hero_id(1);
	(*pi_a.add_hero_list()).set_hero_id(2);
	(*pi_b.add_hero_list()).set_hero_id(1);
	(*pi_b.add_hero_list()).set_hero_id(2);

	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, bi.playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, bi.playerB.RoleID);

	mRuningBattle[bi.playerA.RoleID] = bi;
	mRuningBattle[bi.playerB.RoleID] = bi;
}

void CFrameSyncGameModule::SendFrameFinishCmmand(const BattleInfo& bi)
{
	NFMsg::NtfGCBattleFrameFinish ret;
	ret.set_timestamp(miNowTime);

	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, bi.playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, bi.playerB.RoleID);
}

//EGMI_NTF_CG_BATTLE_FRAMECOMMAND
void CFrameSyncGameModule::OnNtfCGBattleFrameCommandProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGUID nClientID;
	NFMsg::NtfCGBattleFrameCommand xMsg;
	if (!m_pNetModule->ReceivePB(nMsgID, msg, nLen, xMsg, nClientID))
	{
		return;
	}

	NFMsg::NtfGCBattleFrameCommand cmd;
	*cmd.mutable_role_id() = xMsg.role_id();
	cmd.set_skillid(xMsg.skillid());

	auto it = mRuningBattle.find(NFINetModule::PBToNF(xMsg.role_id()));
	if (it != mRuningBattle.end())
	{
		BattleInfo& bi = it->second;
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, cmd, bi.playerA.RoleID);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, cmd, bi.playerB.RoleID);
	}
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
	 miNowTime = NFGetTime();

	 if (miNowTime - miLastTime < FRAMESYNC_TIMESPAN)
		 return;

	 miLastTime = miNowTime;

	 //process frame command EGMI_NTF_GC_BATTLE_FRAMEFINISH
	 RuningBattleIterator begin = mRuningBattle.cbegin();
	 RuningBattleIterator end = mRuningBattle.cend();

	 for (; begin != end; ++begin)
	 {
		 const BattleInfo& bi = begin->second;
		 SendFrameFinishCmmand(bi);
	 }
 }
