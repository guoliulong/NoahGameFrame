
#ifndef IFRAME_SYNC_GAME_MODULE_H
#define IFRAME_SYNC_GAME_MODULE_H

#include "NFComm/NFPluginModule/NFIModule.h"
class IFrameSyncGameModule :
	public NFIModule
{

public:
	virtual void OnClienLeaveGame(NFGUID roleId) = 0;

};

#endif