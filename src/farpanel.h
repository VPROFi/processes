#ifndef __FARPANEL_H__
#define __FARPANEL_H__

#include <farplug-wide.h>
#include <memory>
#include "plugincfg.h"

#define NO_PANEL_INDEX (PanelIndex)(-1)

struct PanelData {
	struct PanelMode panelModesArray[PanelModeMax];
	struct KeyBarTitles keyBar;
	struct OpenPluginInfo openInfo;
	PanelData() {
		memset(panelModesArray, 0, sizeof(panelModesArray));
		memset(&keyBar, 0, sizeof(keyBar));
		memset(&openInfo, 0, sizeof(openInfo));
		openInfo.StructSize = sizeof(OpenPluginInfo);
		openInfo.PanelModesArray = panelModesArray;
		openInfo.PanelModesNumber = ARRAYSIZE(panelModesArray);
		openInfo.KeyBar = &keyBar;
	};
};

class FarPanel: public PluginCfg {
private:
	PanelIndex index;
	std::unique_ptr<PanelData> data;
public:
	virtual int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) = 0;
	virtual int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber, int opMode) = 0;
	virtual void GetOpenPluginInfo(struct OpenPluginInfo * info);
	virtual void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber);
	virtual int SetFindList(const struct PluginPanelItem *panelItem, int itemsNumber);
	virtual int SetDirectory(const wchar_t *dir, int opMode);
	virtual int DeleteFiles(struct PluginPanelItem *panelItem, int itemsNumber, int opMode);
	virtual int GetFiles(struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode);
	virtual int ProcessEvent(int event, void *param);

	virtual bool Valid(void) { return index != NO_PANEL_INDEX; };
	virtual void UdpatePanelInfo(void);

	const wchar_t * GetPanelTitle(void);
	const wchar_t * GetPanelTitleKey(int key, unsigned int controlState = 0) const;

	OpenPluginInfo & GetOpenPluginInfo(void) { return data->openInfo; };
	typedef PanelMode PanelModes[PanelModeMax];
	PanelModes & GetPanelModesArray(void) { return data->panelModesArray; };

	bool IsPanelProcessKey(int key, unsigned int controlState) const;

	FarPanel(uint32_t index);
	FarPanel();
	virtual ~FarPanel();
};

#endif /* __FARPANEL_H__ */
