#include "procplugin.h"
#include "procpanel.h"

#include <cassert>

#include <common/log.h>

#include <memory>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "procplugin.cpp"

ProcPlugin::ProcPlugin(const PluginStartupInfo * info):
	Plugin(info)
{
	LOG_INFO("\n");
	processes = std::make_unique<Processes>();
}

ProcPlugin::~ProcPlugin()
{
	LOG_INFO("\n");
}

HANDLE ProcPlugin::OpenFilePlugin(const wchar_t * name,const unsigned char * data, int dataSize, int opMode)
{
	return INVALID_HANDLE_VALUE;
}

HANDLE ProcPlugin::OpenPlugin(int openFrom, INT_PTR item)
{
	if( openFrom == OPEN_DISKMENU || openFrom == OPEN_PLUGINSMENU ) {
		auto process = std::make_unique<ProcPanel>(ProcessPanelIndex, processes);
		if( process->Valid() ) {
			panel.push_back(std::move(process));
			return panel.back().get();
		}
	}
	return INVALID_HANDLE_VALUE;
}

void ProcPlugin::ClosePlugin(HANDLE hPlugin)
{
	LOG_INFO("hPlugin %p\n", hPlugin);
	for( auto it = panel.begin(); it != panel.end(); it++ )
		if( (*it).get() == hPlugin ) {
			panel.erase(it);
			break;
		}
}

int ProcPlugin::GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber, int opMode)
{
	LOG_INFO("\n");
	return static_cast<FarPanel *>(hPlugin)->GetFindData(pPanelItem, pItemsNumber, opMode);
}
