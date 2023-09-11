#include "farpanel.h"
#include "lng.h"

#include <string>
#include <cassert>
#include <utils.h>

#include <common/log.h>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "farpanel.cpp"

FarPanel::FarPanel():
	index(NO_PANEL_INDEX),
	data(nullptr)
{
	LOG_INFO("index NO_PANEL_INDEX\n");
}

FarPanel::FarPanel(uint32_t index_):
	index(static_cast<PanelIndex>(index_)),
	data(std::make_unique<PanelData>())
{
	LOG_INFO("index %u\n", index);
	FillPanelData(data.get(), index);
}

void FarPanel::UdpatePanelInfo(void)
{
	LOG_INFO("\n");
	if( Valid() )
		FillPanelData(data.get(), index);
};

FarPanel::~FarPanel()
{
	LOG_INFO("\n");
}

void FarPanel::GetOpenPluginInfo(struct OpenPluginInfo * info)
{
	LOG_INFO("index %u\n", index);
	ReloadPanelString(data.get(), index);
	*info = data->openInfo;
}

const wchar_t * FarPanel::GetPanelTitle(void)
{
	ReloadPanelString(data.get(), index);
	return data->openInfo.PanelTitle;
}

int FarPanel::SetFindList(const struct PluginPanelItem *panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	return int(false);
}

int FarPanel::SetDirectory(const wchar_t *dir, int opMode)
{
	LOG_INFO("\n");
	return int(false);
}

int FarPanel::DeleteFiles(struct PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
	LOG_INFO("\n");
	return int(false);
}

int FarPanel::GetFiles(struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode)
{
	LOG_INFO("\n");
	return int(false);
}

int FarPanel::ProcessEvent(int event, void *param)
{
	LOG_INFO("\n");
	return int(false);
}

const wchar_t * FarPanel::GetPanelTitleKey(int key, unsigned int controlState) const
{
	if( !(key >= VK_F1 && key <= VK_F12) )
		return nullptr;

	auto keyNum = key - VK_F1;
	
	switch( controlState ) {
	case 0:
		return data->openInfo.KeyBar->Titles[keyNum];
	case PKF_SHIFT:
		return data->openInfo.KeyBar->ShiftTitles[keyNum];
	case PKF_CONTROL:
		return data->openInfo.KeyBar->CtrlTitles[keyNum];
	case PKF_ALT:
		return data->openInfo.KeyBar->AltTitles[keyNum];
	case (PKF_CONTROL|PKF_SHIFT):
		return data->openInfo.KeyBar->CtrlShiftTitles[keyNum];
	case (PKF_ALT|PKF_SHIFT):
		return data->openInfo.KeyBar->AltShiftTitles[keyNum];
	case (PKF_CONTROL|PKF_ALT):
		return data->openInfo.KeyBar->CtrlAltTitles[keyNum];
	};

	return nullptr;
}

bool FarPanel::IsPanelProcessKey(int key, unsigned int controlState) const
{
	return GetPanelTitleKey(key, controlState) != 0;
}

void FarPanel::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber)
{
	LOG_INFO("\n");
	while( itemsNumber-- ) {

		if( (panelItem+itemsNumber)->Owner )
			free((void *)(panelItem+itemsNumber)->Owner);
		if( (panelItem+itemsNumber)->Group )
			free((void *)(panelItem+itemsNumber)->Group);

		while( (panelItem+itemsNumber)->CustomColumnNumber-- )
			free((void *)(panelItem+itemsNumber)->CustomColumnData[(panelItem+itemsNumber)->CustomColumnNumber]);
		free((void *)(panelItem+itemsNumber)->CustomColumnData);

		if( (panelItem+itemsNumber)->Flags & PPIF_USERDATA && (panelItem+itemsNumber)->UserData )
			free((void *)(panelItem+itemsNumber)->UserData);

	}
	free((void *)panelItem);
}
