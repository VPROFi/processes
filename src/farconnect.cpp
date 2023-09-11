#include "procplugin.h"

#include <common/log.h>
#include <common/errname.h>
#include <cassert>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "farconnect.cpp"

// plugin
static class ProcPlugin * gPlugin = 0;

// export for processes
int RootExec(const char * cmd, int flags)
{
	assert( gPlugin != 0 );
	if( !gPlugin )
		return -1;
	std::string _acmd(cmd);
	std::wstring _cmd(_acmd.begin(), _acmd.end());
	int res = gPlugin->FSF.Execute(_cmd.c_str(), flags | EF_SUDO);
	LOG_INFO("exec \"%s\" return %d\n", cmd, res);
	return res;
}

int Exec(const char * cmd, int flags)
{
	assert( gPlugin != 0 );
	if( !gPlugin )
		return -1;
	std::string _acmd(cmd);
	std::wstring _cmd(_acmd.begin(), _acmd.end());
	int res = gPlugin->FSF.Execute(_cmd.c_str(), flags);
	LOG_INFO("exec \"%s\" return %d\n", cmd, res);
	return res;
}

SHAREDSYMBOL int WINAPI _export ConfigureW(int itemNumber)
{
	LOG_INFO("\n");
	assert( gPlugin != 0 );
	return gPlugin->Configure(itemNumber);
}

SHAREDSYMBOL int WINAPI _export ProcessKeyW(HANDLE hPlugin,int key,unsigned int controlState)
{
	return gPlugin->ProcessKey(hPlugin, key, controlState);
}

#include <time.h>

SHAREDSYMBOL int WINAPI _export ProcessEventW(HANDLE hPlugin,int event,void *param)
{
	switch(event) {
	case FE_CHANGEVIEWMODE:
		LOG_INFO(":FE_CHANGEVIEWMODE \"%S\"\n", (const wchar_t *)param);
		//res = FALSE;
		break;
	case FE_REDRAW:
		LOG_INFO(":FE_REDRAW %p\n", param);
		//res = FALSE; // redraw
		//res = TRUE; // no redraw
		break;
	case FE_IDLE:
		{
		// t = clock();
		//LOG_INFO(":FE_IDLE %p\n", param);
		static clock_t t;
		clock_t t1 = clock();
		LOG_INFO("FE_IDLE %p time %f\n", param, ((double)t1 - t) / CLOCKS_PER_SEC);
		t = t1;
		//res = FALSE;
		}
		break;
	case FE_CLOSE:
		LOG_INFO(":FE_CLOSE %p\n", param);
		//res = FALSE; // close panel
		// res = TRUE; // no close panel
		break;
	case FE_BREAK:
		// Ctrl-Break is pressed
		// Processing of this event is performed in separate thread,
		// plugin must not use FAR service functions.
#if INTPTR_MAX == INT32_MAX
		LOG_INFO(":FE_BREAK CTRL_BREAK_EVENT %llu\n", param);
#elif INTPTR_MAX == INT64_MAX
		LOG_INFO(":FE_BREAK CTRL_BREAK_EVENT %lu\n", param);
#else
    #error "Environment not 32 or 64-bit."
#endif
		//res = FALSE;
		break;
	case FE_COMMAND:
		LOG_INFO(":FE_COMMAND \"%S\"\n", (const wchar_t *)param);
		//res = FALSE; // allow standard command execution
		//res = TRUE; //  TRUE if it is going to process the command internally.
		break;
	case FE_GOTFOCUS:
		LOG_INFO(":FE_GOTFOCUS %p\n", param);
		//res = FALSE;
		break;
	case FE_KILLFOCUS:
		LOG_INFO(":FE_KILLFOCUS %p\n", param);
		//res = FALSE;
		break;
	default:
		LOG_INFO(":UNKNOWN %p\n", param);
		break;
	};
	return gPlugin->ProcessEvent(hPlugin, event, param);
}

SHAREDSYMBOL void WINAPI _export FreeFindDataW(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber)
{
	LOG_INFO("hPlugin %p pPanelItem %p ItemsNumber %u\n", hPlugin, PanelItem, ItemsNumber);
	gPlugin->FreeFindData(hPlugin, PanelItem, ItemsNumber);
	return;
}

SHAREDSYMBOL int WINAPI _export GetFindDataW(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int opMode)
{
	LOG_INFO("hPlugin %p OpMode %d *pPanelItem %p *pItemsNumber %u\n", hPlugin, opMode, *pPanelItem, *pItemsNumber);
	gPlugin->GetFindData(hPlugin, pPanelItem, pItemsNumber, opMode);
	return TRUE;
}

SHAREDSYMBOL void WINAPI _export GetOpenPluginInfoW(HANDLE hPlugin, struct OpenPluginInfo *info)
{
	LOG_INFO("hPlugin %p info %p\n", hPlugin, info);
	gPlugin->GetOpenPluginInfo(hPlugin, info);	
}

SHAREDSYMBOL HANDLE WINAPI _export OpenPluginW(int openFrom, INT_PTR item)
{
	if( openFrom == OPEN_COMMANDLINE && item )
		LOG_INFO("openFrom=%u, item=%S\n", openFrom, reinterpret_cast<const wchar_t *>(item));
	else
		LOG_INFO("openFrom=%u, item=%u\n", openFrom, item);
	return gPlugin->OpenPlugin(openFrom, item);
}

SHAREDSYMBOL int WINAPI _export SetFindListW(HANDLE hPlugin,const struct PluginPanelItem * panelItem,int itemsNumber)
{
	LOG_INFO("hPlugin %p panelItem %p itemsNumber %d\n", hPlugin, panelItem, itemsNumber);
	return gPlugin->SetFindList(hPlugin, panelItem, itemsNumber);
}

// This function is called only for plugins that implement virtual file systems. 
// For this it is necessary to remove the OPIF_REALNAMES flag when GetOpenPluginInfo is called, 
// otherwise this function will never be called.
SHAREDSYMBOL int WINAPI _export GetFilesW(HANDLE hPlugin,struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode)
{
	LOG_INFO("hPlugin %p panelItem %p itemsNumber %d move %d destPath %p opMode %d\n", hPlugin, panelItem, itemsNumber, move, destPath, opMode);
	return gPlugin->GetFiles(hPlugin, panelItem, itemsNumber, move, destPath, opMode);
}

SHAREDSYMBOL void WINAPI _export ClosePluginW(HANDLE hPlugin)
{
	LOG_INFO("hPlugin 0x%p\n", hPlugin);
	gPlugin->ClosePlugin(hPlugin);
}

SHAREDSYMBOL void WINAPI _export GetPluginInfoW(struct PluginInfo *info)
{
	LOG_INFO("(struct PluginInfo *info = %p)\n", info);
	gPlugin->GetPluginInfo(info);
}

SHAREDSYMBOL void WINAPI _export SetStartupInfoW(const struct PluginStartupInfo * info)
{
	assert( gPlugin == 0 );
	gPlugin = new ProcPlugin(info);
}

SHAREDSYMBOL HANDLE WINAPI _export OpenFilePluginW(const wchar_t *name,const unsigned char *data,int dataSize,int opMode)
{
	LOG_INFO("OpenFilePluginW(Name=%S,*Data=%p,DataSize=%d,OpMode=%d)\n", name, data, dataSize, opMode);
	return gPlugin->OpenFilePlugin(name, data, dataSize, opMode);
}

SHAREDSYMBOL int WINAPI _export SetDirectoryW(HANDLE hPlugin, const wchar_t *dir, int opMode)
{
	LOG_INFO("SetDirectoryW(hPlugin=%p, Dir=%S, OpMode=%d)\n", hPlugin, dir, opMode);
	return gPlugin->SetDirectory(hPlugin, dir, opMode);
}

SHAREDSYMBOL int WINAPI _export DeleteFilesW(HANDLE hPlugin, struct PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
	LOG_INFO("DeleteFilesW(hPlugin=%p, PanelItem=%p, ItemsNumber %d, OpMode=%d)\n", hPlugin, panelItem, itemsNumber, opMode);
	return gPlugin->DeleteFiles(hPlugin, panelItem, itemsNumber, opMode);
}

SHAREDSYMBOL int WINAPI _export GetMinFarVersionW()
{
//	LOG_INFO("return 0x%08X\n", FARMANAGERVERSION);
	return FARMANAGERVERSION;
}

SHAREDSYMBOL void WINAPI _export ExitFARW()
{
	LOG_INFO("\n");
	delete gPlugin;
	gPlugin = 0;
}
