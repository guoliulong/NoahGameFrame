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

	/*NFMsg::AckRoleLiteInfoList xAckRoleLiteInfoList;
	NFMsg::RoleLiteInfo* pData = xAckRoleLiteInfoList.add_char_data();
	pData->mutable_id()->CopyFrom(NFINetModule::NFToPB(m_pKernelModule->CreateGUID()));
	pData->set_career(xMsg.career());
	pData->set_sex(xMsg.sex());
	pData->set_race(xMsg.race());
	pData->set_noob_name(xMsg.noob_name());
	pData->set_game_id(xMsg.game_id());
	pData->set_role_level(1);
	pData->set_delete_time(0);
	pData->set_reg_time(0);
	pData->set_last_offline_time(0);
	pData->set_last_offline_ip(0);
	pData->set_view_record("");*/

	//m_pNetModule->SendMsgPB(NFMsg::EGMI_ACK_ROLE_LIST, xAckRoleLiteInfoList, nSockIndex, nClientID);
}
