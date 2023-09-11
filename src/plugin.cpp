#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "plugin.h"

#include <utils.h>

#include <cassert>

#include <common/log.h>
#include <common/errname.h>

#include <memory>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "plugin.cpp"

struct PluginStartupInfo Plugin::psi = {0};
struct FarStandardFunctions Plugin::FSF = {0};

Plugin::Plugin(const PluginStartupInfo * info)
{
	assert( (unsigned int)info->StructSize >= sizeof(PluginStartupInfo) );
	assert( psi.StructSize == 0 );
	psi=*info;

	assert( (unsigned int)info->FSF->StructSize >= sizeof(FarStandardFunctions) );
	assert( FSF.StructSize == 0 );
	FSF=*info->FSF;

	psi.FSF=&FSF;

	cfg = new PluginCfg();

	config_change = false;

	LOG_INFO("\n");
}

Plugin::~Plugin()
{
	LOG_INFO("\n");

	panel.clear();

	delete cfg;
	cfg = nullptr;

	memset(&psi, 0, sizeof(psi));
	memset(&FSF, 0, sizeof(FSF));
}

int Plugin::Configure(int itemNumber)
{
	assert( cfg != 0 );
	config_change = cfg->Configure(itemNumber) != 0;
	return int(config_change);
}

void Plugin::GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *info)
{
	LOG_INFO("GetOpenPluginInfo(hPlugin = %p)\n", hPlugin);
	assert( hPlugin && hPlugin != INVALID_HANDLE_VALUE );

	if( config_change ) {
		config_change = false;
		for( auto & item : panel )
			item->UdpatePanelInfo();
	}

	static_cast<FarPanel *>(hPlugin)->GetOpenPluginInfo(info);
}

void Plugin::GetPluginInfo(struct PluginInfo *info)
{
	LOG_INFO("\n");
	cfg->GetPluginInfo(info);
}

int Plugin::GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber, int opMode)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->GetFindData(pPanelItem, pItemsNumber, opMode);
}

void Plugin::FreeFindData(HANDLE hPlugin,struct PluginPanelItem * panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->FreeFindData(panelItem, itemsNumber);
}

int Plugin::ProcessKey(HANDLE hPlugin,int key,unsigned int controlState)
{
	LOG_INFO("hPlugin: %p key 0x%08X VK_F2 0x%08X\n", hPlugin, key, VK_F2);

	bool redraw = false;
	int res = static_cast<FarPanel *>(hPlugin)->ProcessKey(hPlugin, key, controlState, redraw);

	if( redraw ) {
		LOG_INFO("Plugin::ProcessKey() redraw\n");
		psi.Control(hPlugin, FCTL_UPDATEPANEL, TRUE, 0);
		psi.Control(hPlugin, FCTL_REDRAWPANEL, 0, 0);
	}
	return res;
}

int Plugin::SetDirectory(HANDLE hPlugin, const wchar_t *dir, int opMode)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->SetDirectory(dir, opMode);
}

int Plugin::SetFindList(HANDLE hPlugin, const struct PluginPanelItem *panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->SetFindList(panelItem, itemsNumber);
}

int Plugin::DeleteFiles(HANDLE hPlugin, struct PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->DeleteFiles(panelItem, itemsNumber, opMode);
}

int Plugin::GetFiles(HANDLE hPlugin,struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->GetFiles(panelItem, itemsNumber, move, destPath, opMode);
}

int Plugin::ProcessEvent(HANDLE hPlugin,int event,void *param)
{
	return static_cast<FarPanel *>(hPlugin)->ProcessEvent(event, param);
}
