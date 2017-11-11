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
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_NTF_CG_BATTLE_CHECKMD5, this, &CFrameSyncGameModule::OnNtfCGBattleCheckMd5Process);
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_REQ_BATTLE_PING, this, &CFrameSyncGameModule::OnReqBattlePingProcess);
	
	return true;
}
bool CFrameSyncGameModule::ReadyExecute()
{
	miLastTime = NFGetTime();
	return true;
}
bool CFrameSyncGameModule::Execute()
{
	//int64_t t = NFGetTime();

	ProcessMatch();
	ProcessFrameSync();

	//std::cout << (NFGetTime() - t) << std::endl;

	return true;
}

void CFrameSyncGameModule::OnClienLeaveGame(NFGUID roleId)
{
	//if matching
	size_t count = mMatchingPlayers.size();
	for (size_t i = 0; i < count; ++i)
	{
		if (mMatchingPlayers[i].RoleID == roleId)
		{
			mMatchingPlayers.erase(mMatchingPlayers.begin() + i);
			return;
		}
	}

	NFMsg::NtfGCBattleFinish ret;
	ret.set_timestamp(miNowTime);
	//if loading
	auto it = mLoadingBattles.find(roleId);
	if (it != mLoadingBattles.end())
	{
		if (it->second->playerA.RoleID == roleId)
		{
			m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FINISH, ret, it->second->playerB.RoleID);
		}
		else
		{
			m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FINISH, ret, it->second->playerA.RoleID);
		}

		NFGUID aRoleID = it->second->playerA.RoleID;
		NFGUID bRoleID = it->second->playerB.RoleID;

		mLoadingBattles.erase(aRoleID);
		mLoadingBattles.erase(bRoleID);

		return;
	}

	//if battling
	auto it2 = mRoleID2BattleInfo.find(roleId);
	if (it2 != mRoleID2BattleInfo.end())
	{
		auto battleInfo = it2->second;

		mRoleID2BattleInfo.erase(battleInfo->playerA.RoleID);
		mRoleID2BattleInfo.erase(battleInfo->playerB.RoleID);
		mRuningBattles.erase(battleInfo);
		if (battleInfo->playerA.RoleID == roleId)
		{
			m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FINISH, ret, battleInfo->playerB.RoleID);
		}
		else
		{
			m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FINISH, ret, battleInfo->playerA.RoleID);
		}
		return;
	}
}

void CFrameSyncGameModule::OnReqBattlePingProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGUID nClientID;
	NFMsg::ReqBattlePing xMsg;
	if (!m_pNetModule->ReceivePB(nMsgID, msg, nLen, xMsg, nClientID))
	{
		return;
	}

	NFMsg::AckBattlePing ret;
	ret.set_frameindex(xMsg.frameindex());

	 m_pNetModule->SendMsgPB(NFMsg::EGMI_ACK_BATTLE_PING, ret,nSockIndex,nClientID);
}

//EGMI_NTF_CG_BATTLE_CHECKMD5
void CFrameSyncGameModule::OnNtfCGBattleCheckMd5Process(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGUID nClientID;
	NFMsg::NtfCGBattleCheckMd5 xMsg;
	if (!m_pNetModule->ReceivePB(nMsgID, msg, nLen, xMsg, nClientID))
	{
		return;
	}

	NFGUID roleId = NFINetModule::PBToNF(xMsg.role_id());
	auto it = mRoleID2BattleInfo.find(roleId);

	if (it != mRoleID2BattleInfo.end())
	{
		auto &pbi = it->second;
		if (pbi->playerA.RoleID == roleId) //这个角色是A
		{
			auto it_data = pbi->playerB.Md5Map.find(xMsg.frameindex());
			if (it_data != pbi->playerB.Md5Map.end())
			{
				NFMsg::NtfGCBattleCheckMd5 ret;
				ret.set_frameindex(xMsg.frameindex());

				if (it_data->second == xMsg.md5())
				{
					//todo success
					ret.set_event_code(NFMsg::EGEC_SUCCESS);
				}
				else
				{
					ret.set_event_code(NFMsg::EGEC_UNKOWN_ERROR);
				}

				m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_CHECKMD5, ret, pbi->playerA.RoleID);
				m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_CHECKMD5, ret, pbi->playerB.RoleID);

				pbi->playerB.Md5Map.erase(it_data);
			}
			else
			{
				//没有找到说明第一个来的
				pbi->playerA.Md5Map[xMsg.frameindex()] = xMsg.md5();
			}
		}

		if (pbi->playerB.RoleID == roleId)//这个角色是B
		{
			auto it_data = pbi->playerA.Md5Map.find(xMsg.frameindex());
			if (it_data != pbi->playerA.Md5Map.end())
			{
				NFMsg::NtfGCBattleCheckMd5 ret;
				ret.set_frameindex(xMsg.frameindex());

				if (it_data->second == xMsg.md5())
				{
					//todo success
					ret.set_event_code(NFMsg::EGEC_SUCCESS);
				}
				else
				{
					ret.set_event_code(NFMsg::EGEC_UNKOWN_ERROR);
				}
				m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_CHECKMD5, ret, pbi->playerA.RoleID);
				m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_CHECKMD5, ret, pbi->playerB.RoleID);
				pbi->playerA.Md5Map.erase(it_data);
			}
			else
			{
				//没有找到说明第一个来的
				pbi->playerB.Md5Map[xMsg.frameindex()] = xMsg.md5();
				
			}
		}
	}
	else
	{
		//战斗都没有了,还发md5
	}
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

		NFMsg::NtfGCBattleFrameStart ret_FrameStart;
		ret_FrameStart.set_frameindex(pbi->frameIndex);
		ret_FrameStart.set_timestamp(miNowTime);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMESTART, ret_FrameStart, pbi->playerA.RoleID);
		m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMESTART, ret_FrameStart, pbi->playerB.RoleID);
	}
}

void CFrameSyncGameModule::SendFrameFinishCmmand(const NF_SHARE_PTR<BattleInfo> pbi)
{
	NFMsg::NtfGCBattleFrameFinish ret;
	ret.set_timestamp(miNowTime);
	ret.set_frameindex(pbi->frameIndex);
	ret.set_timestamp(miNowTime);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMEFINISH, ret, pbi->playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMEFINISH, ret, pbi->playerB.RoleID);

	NFMsg::NtfGCBattleFrameStart ret_FrameStart;
	ret_FrameStart.set_frameindex(pbi->frameIndex + 1);
	ret_FrameStart.set_timestamp(miNowTime);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMESTART, ret_FrameStart, pbi->playerA.RoleID);
	m_pGameServerNet_ServerModule->SendMsgPBToGate(NFMsg::EGMI_NTF_GC_BATTLE_FRAMESTART, ret_FrameStart, pbi->playerB.RoleID);
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
	cmd.set_skill_slot_index(xMsg.skill_slot_index());
	
	auto it = mRoleID2BattleInfo.find(NFINetModule::PBToNF(xMsg.role_id()));
	if (it != mRoleID2BattleInfo.end())
	{
		NF_SHARE_PTR<BattleInfo> pBi = it->second;
		cmd.set_frameindex(pBi->frameIndex);
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
	 //std::cout << miNowTime - miLastFrameTime << std::endl;
	 //miLastFrameTime = miNowTime;

	 if (miNowTime - miLastTime <= FRAMESYNC_TIMESPAN)
		 return;

	 miLastTime += FRAMESYNC_TIMESPAN;
	 RuningBattleIterator begin = mRuningBattles.cbegin();
	 RuningBattleIterator end = mRuningBattles.cend();
	 for (; begin != end; ++begin)
	 {
		 const  NF_SHARE_PTR<BattleInfo> pbi = *begin;
		 SendFrameFinishCmmand(pbi);
		 ++pbi->frameIndex;
	 }
 }
