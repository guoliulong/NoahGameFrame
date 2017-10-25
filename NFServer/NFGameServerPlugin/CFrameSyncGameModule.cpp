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
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_REQ_BATTLE_START, this, &CFrameSyncGameModule::OnReqBattleStartProcess);
	
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

void CFrameSyncGameModule::ProcessMatchSuccess(NF_SHARE_PTR<BattleInfo> pBI)
{
	NFMsg::AckBattleMatch ret;
	ret.set_issuccessed(true);
	NFMsg::PlayerInfo& pi_a = *ret.mutable_playera();
	*pi_a.mutable_role_id() = NFINetModule::NFToPB(pBI->playerA.RoleID);
	*pi_a.mutable_player_name() = "playerA";
	NFMsg::PlayerInfo& pi_b = *ret.mutable_playerb();
	*pi_b.mutable_role_id() = NFINetModule::NFToPB(pBI->playerB.RoleID);
	*pi_b.mutable_player_name() = "playerB";
	//set heros
	(*pi_a.add_hero_list()).set_hero_id(1);
	(*pi_a.add_hero_list()).set_hero_id(2);
	(*pi_b.add_hero_list()).set_hero_id(1);
	(*pi_b.add_hero_list()).set_hero_id(2);

	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, pBI->playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_MATCH, ret, pBI->playerB.RoleID);

	mLoadingBattles[pBI->playerA.RoleID] = pBI;
	mLoadingBattles[pBI->playerB.RoleID] = pBI;
}

//EGMI_REQ_BATTLE_START
void CFrameSyncGameModule::OnReqBattleStartProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGUID nClientID;
	NFMsg::ReqBattleStart xMsg;
	if (!m_pNetModule->ReceivePB(nMsgID, msg, nLen, xMsg, nClientID))
	{
		return;
	}
	ProcessReadyStart(NFINetModule::PBToNF(xMsg.role_id()));
}

//
void CFrameSyncGameModule::ProcessReadyStart(NFGUID RoleID)
{
	auto it = mLoadingBattles.find(RoleID);
	if (it != mLoadingBattles.end())
	{
		NF_SHARE_PTR<BattleInfo> pbi = it->second;

		if (pbi->playerA.RoleID == RoleID)
		{
			pbi->playerA.IsReady = true;
		}
		if (pbi->playerB.RoleID == RoleID)
		{
			pbi->playerB.IsReady = true;
		}

		if(pbi->playerA.IsReady && pbi->playerB.IsReady)
		{ 
			mLoadingBattles.erase(pbi->playerA.RoleID);
			mLoadingBattles.erase(pbi->playerB.RoleID);
		}
		else
		{
			return;
		}

		mRuningBattles.insert(pbi);
		mRoleID2BattleInfo[pbi->playerA.RoleID] = pbi;
		mRoleID2BattleInfo[pbi->playerB.RoleID] = pbi;

		NFMsg::AckBattleStart ret;
		ret.set_event_code(NFMsg::EGEC_SUCCESS);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_START, ret, pbi->playerA.RoleID);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_ACK_BATTLE_START, ret, pbi->playerB.RoleID);
	}
}

void CFrameSyncGameModule::SendFrameFinishCmmand(const NF_SHARE_PTR<BattleInfo> pbi)
{
	NFMsg::NtfGCBattleFrameFinish ret;
	ret.set_timestamp(miNowTime);

	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMEFINISH, ret, pbi->playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMEFINISH, ret, pbi->playerB.RoleID);
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

	auto it = mRoleID2BattleInfo.find(NFINetModule::PBToNF(xMsg.role_id()));
	if (it != mRoleID2BattleInfo.end())
	{
		NF_SHARE_PTR<BattleInfo> pBi = it->second;
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMECOMMAND, cmd, pBi->playerA.RoleID);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMECOMMAND, cmd, pBi->playerB.RoleID);
	}
}

 void CFrameSyncGameModule::ProcessMatch() 
 {
	 while (mMatchingPlayers.size() > 1)
	 {
		 NF_SHARE_PTR<BattleInfo> pBi = NF_SHARE_PTR<BattleInfo>(NF_NEW BattleInfo());

		 pBi->playerA = mMatchingPlayers[0];
		 pBi->playerB = mMatchingPlayers[1];
		 mMatchingPlayers.erase(mMatchingPlayers.begin());
		 mMatchingPlayers.erase(mMatchingPlayers.begin());
		 ProcessMatchSuccess(pBi);
	 }
 }


 //process frame command EGMI_NTF_GC_BATTLE_FRAMEFINISH
 void CFrameSyncGameModule::ProcessFrameSync()
 {
	 miNowTime = NFGetTime();
	 if (miNowTime - miLastTime < FRAMESYNC_TIMESPAN)
		 return;

	 miLastTime = miNowTime;
	 RuningBattleIterator begin = mRuningBattles.cbegin();
	 RuningBattleIterator end = mRuningBattles.cend();
	 for (; begin != end; ++begin)
	 {
		 const  NF_SHARE_PTR<BattleInfo> pbi = *begin;
		 SendFrameFinishCmmand(pbi);
	 }
 }
