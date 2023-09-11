#ifndef __CONFIGPLUGIN_H__
#define __CONFIGPLUGIN_H__

#include <farplug-wide.h>
#include <farkeys.h>
#include <map>
#include <string>
#include "lng.h"
#include "farapi.h"
#include <KeyFileHelper.h>

enum {
	PanelModeBrief,
	PanelModeMedium,
	PanelModeFull,
	PanelModeWide,
	PanelModeDetailed,
	PanelModeDiz,
	PanelModeLongDiz,
	PanelModeOwners,
	PanelModeLinks,
	PanelModeAlternative,
	PanelModeMax,
};

typedef struct {
	const wchar_t * statusColumnTypes;
	const wchar_t * statusColumnWidths;
	const wchar_t * columnTypes[2];
	const wchar_t * columnWidths[2];
	const wchar_t * columnTitles[2][15];
	uint32_t keyBarTitles[12];
	uint32_t keyBarShiftTitles[12];
	uint32_t keyBarCtrlTitles[12];
	uint32_t panelTitle;
	uint32_t format;
	uint32_t flags;
	int startSortMode;
	int startSortOrder;
} CfgDefaults;

typedef enum {
	ProcessPanelIndex,
	MaxPanelIndex
} PanelIndex;

typedef struct {
	const wchar_t fieldType[sizeof("C100")];
	uint32_t fieldWidth;
	uint32_t lngId;
} CgfFields;

class PluginCfg : public FarApi {

	private:
		static std::map<PanelIndex, CfgDefaults> def;
		static std::map<const std::wstring,CgfFields> fields;

		static size_t init;

		const char * GetPanelName(PanelIndex index) const;

		static bool logEnable;

		friend LONG_PTR WINAPI CfgDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2);

		void SaveConfig(void) const;
		void FillFields(HANDLE hDlg, int listIndex, int index);
		void GetFieldsFromConfig(CfgDefaults & item, KeyFileReadHelper & kfrh, const char * name, int index, const char * prefix);

	public:
		explicit PluginCfg();
		~PluginCfg();

		static bool processAddToDisksMenu;
		static bool processAddToPluginsMenu;
		std::wstring prefix;

		void FillPanelData(struct PanelData * data, PanelIndex index);
		void ReloadPanelKeyBar(struct PanelData * data, PanelIndex index);
		void ReloadPanelString(struct PanelData * data, PanelIndex index);

		void GetPluginInfo(struct PluginInfo *info);
		int Configure(int itemNumber);
};

#endif /* __CONFIGPLUGIN_H__ */
