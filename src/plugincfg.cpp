#include "plugincfg.h"
#include "procplugin.h"
#include "lng.h"
#include "fardialog.h"
#include "farpanel.h"

#include <string>
#include <cassert>


#include <utils.h>

#include <common/log.h>

#define LOG_SOURCE_FILE "plugincfg.cpp"
#define LOG_MAX_PATH 256

//static char initial_log[LOG_MAX_PATH] = {'/','t','m','p','/','f','a','r','2','l','.','p','r','o','c','e','s','s','.','l','o','g',0};
static char initial_log[LOG_MAX_PATH] = {'/','d','e','v','/','n','u','l','l',0};
const char * LOG_FILE = initial_log;
//static_assert( sizeof("/dev/null") < LOG_MAX_PATH );

#define INI_LOCATION InMyConfig("plugins/process/config.ini")
#define INI_SECTION "Settings"
#define DEFAULT_PREFIX L"process"

const char * PluginCfg::GetPanelName(PanelIndex index) const
{
	static const char * names[] = {
		"Process",	// ProcessPanelIndex,
		"max"		// MaxPanelIndex
	};
	assert( index < (ARRAYSIZE(names)-1) );
	return names[index];
}

size_t PluginCfg::init = 0;
std::map<PanelIndex, CfgDefaults> PluginCfg::def = {\
		{ProcessPanelIndex, {
		L"N,C0,C1,C2,C3,SF",
		L"0,8,3,5,6,10",
		// module                     N
		// pid                        C0
		// priority                   C1
		// nice                       C2
		// cpu                        C3
		// memory                     SF
		// time+                      C4
		// owner                      O
		{L"N,C0,C1,C2,C3,SF", L"N,C0,C1,C2,C3,SF,C4,O,U"},
		{L"0,8,3,5,6,10", L"0,8,3,5,6,10,35,10,10"},
		{{L"module",L"pid",L"priority",L"nice",L"cpu",L"memory", 0}, {L"module",L"pid",L"priority",L"nice",L"cpu",L"memory",L"time+",L"owner",L"group",0}},
		{0,MEmptyString,0,0,MEmptyString,MEmptyString,MEmptyString,0,0,0,0,0},
		{MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString,MEmptyString},
		{0,0,0,MF4CPUSort,MF5PidSort,0,0,MF8TimeSort,0,0,0,0},
		MPanelProcessTitle,
		MFormatProcessPanel,
		OPIF_USEFILTER|OPIF_USEHIGHLIGHTING|OPIF_SHOWPRESERVECASE|OPIF_ADDDOTS,
		SM_NUMLINKS, 0
		}},
		};

std::map<const std::wstring, CgfFields> PluginCfg::fields = {
//         name         type        width           lngId
	{L"module",   {{'N',0},     0,                0}},
	{L"pid",      {{'C','0',0}, 5,                0}},
	{L"priority", {{'C','1',0}, 3,                0}},
	{L"nice",     {{'C','2',0}, 3,                0}},
	{L"cpu",      {{'C','3',0}, 5,                0}},
	{L"memory",   {{'S','F',0}, 10,               0}},
	{L"time+",    {{'C','4',0}, 35,               0}},
	{L"owner",    {{'O',0},     10,               0}},
	{L"group",    {{'U',0},     10,               0}}
};


bool PluginCfg::logEnable = true;
bool PluginCfg::processAddToDisksMenu = false;
bool PluginCfg::processAddToPluginsMenu = false;

void PluginCfg::ReloadPanelKeyBar(struct PanelData * data, PanelIndex index)
{
	assert( def.find(index) != def.end() );
	auto & cfg = def[index];
	auto keyBar = &data->keyBar.Titles[0];
	for( auto item : cfg.keyBarTitles ) {
		if( item && item < MMaxString )
			*keyBar = (TCHAR*)GetMsg(item);
		keyBar++;
	}

	keyBar = &data->keyBar.ShiftTitles[0];
	for( auto item : cfg.keyBarShiftTitles ) {
		if( item && item < MMaxString )
			*keyBar = (TCHAR*)GetMsg(item);
		keyBar++;
	}

	keyBar = &data->keyBar.CtrlTitles[0];
	for( auto item : cfg.keyBarCtrlTitles ) {
		if( item && item < MMaxString )
			*keyBar = (TCHAR*)GetMsg(item);
		keyBar++;
	}

	constexpr static wchar_t * empty_string = const_cast<wchar_t *>(L"");
	constexpr static wchar_t * defAltkeys[12] = {0,0,empty_string,empty_string,empty_string,empty_string,empty_string,0,0,empty_string,0,0};
	constexpr static wchar_t * defkeys[12] = {empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string,empty_string};
	memmove(data->keyBar.AltTitles, defAltkeys, sizeof(defAltkeys));
	memmove(data->keyBar.CtrlShiftTitles, defkeys, sizeof(defkeys));
	memmove(data->keyBar.AltShiftTitles, defkeys, sizeof(defkeys));
	memmove(data->keyBar.CtrlAltTitles, defkeys, sizeof(defkeys));
}

void PluginCfg::ReloadPanelString(struct PanelData * data, PanelIndex index)
{
	ReloadPanelKeyBar(data, index);
	auto & cfg = def[index];
	data->openInfo.Format=(TCHAR*)GetMsg(cfg.format);
	data->openInfo.PanelTitle=(TCHAR*)GetMsg(cfg.panelTitle);
}

void PluginCfg::FillPanelData(struct PanelData * data, PanelIndex index)
{
	assert( def.find(index) != def.end() );

	auto & cfg = def[index];
	auto & nmodes = data->panelModesArray;
	for( size_t i =0; i < ARRAYSIZE(nmodes); i++ ) {
		nmodes[i].FullScreen = FALSE;
		nmodes[i].DetailedStatus = 0;
		nmodes[i].AlignExtensions = 0;
		nmodes[i].CaseConversion = TRUE;
		nmodes[i].StatusColumnTypes = cfg.statusColumnTypes;
		nmodes[i].StatusColumnWidths = cfg.statusColumnWidths;
	}

	nmodes[4].ColumnTypes = cfg.columnTypes[0];
	nmodes[4].ColumnWidths = cfg.columnWidths[0];
	// TODO
	nmodes[4].ColumnTitles = cfg.columnTitles[0];

	nmodes[5].ColumnTypes =  cfg.columnTypes[1];
	nmodes[5].ColumnWidths = cfg.columnWidths[1];
	// TODO
	nmodes[5].ColumnTitles = cfg.columnTitles[1];
	nmodes[5].FullScreen = TRUE;

	data->openInfo.Flags = cfg.flags;
	data->openInfo.CurDir=_T("");
	data->openInfo.StartPanelMode=_T('4');
	data->openInfo.StartSortMode = cfg.startSortMode;
	data->openInfo.StartSortOrder = cfg.startSortOrder;

	ReloadPanelString(data, index);
}

void PluginCfg::GetFieldsFromConfig(CfgDefaults & item, KeyFileReadHelper & kfrh, const char * name, int index, const char * prefix)
{
	std::string field_prefix(prefix);
	for( size_t i = 0; i < ARRAYSIZE(item.columnTitles[0]); i++ ) {
			std::string field = field_prefix + std::to_string(i);
			std::wstring iniTitle = kfrh.GetString(name, field.c_str(), L"");
			if( !iniTitle.empty() )
				item.columnTitles[index][i] = wcsdup(iniTitle.c_str());
			else {
				if( i != 0 )
					item.columnTitles[index][i] = 0;
				else {
					// no config: fill default values
					for( size_t j = 0; j < ARRAYSIZE(item.columnTitles[0]); j++ )
						if( item.columnTitles[index][j] )
							item.columnTitles[index][j] = wcsdup(item.columnTitles[index][j]);
						else
							break;
					break;
				}
			}
	}
}

PluginCfg::PluginCfg()
{
	LOG_INFO("init %d\n", init);

	if( init++ )
		return;

	LOG_INFO("=== INIT ===\n");

	{
		KeyFileReadSection kfr(INI_LOCATION, INI_SECTION);

		processAddToDisksMenu = (bool)kfr.GetInt("processAddToDisksMenu", true);
		processAddToPluginsMenu = (bool)kfr.GetInt("processAddToPluginsMenu", true);

		prefix = kfr.GetString("prefix", DEFAULT_PREFIX);

		logEnable = (bool)kfr.GetInt("logEnable", false);
	       	if( logEnable ) {
			std::string logfile = kfr.GetString("logfile", initial_log);
			if( logfile.size() < (LOG_MAX_PATH-1) && logfile.size() >= sizeof("/a") )
				memmove(initial_log, logfile.c_str(), logfile.size()+1);
		} else
			memmove(initial_log, "/dev/null", sizeof("/dev/null"));
	}

	KeyFileReadHelper kfrh(INI_LOCATION);
	for( auto & [index, item] : def ) {
		const char * name = GetPanelName(index);

		item.statusColumnTypes = wcsdup(kfrh.GetString(name, "statusColumnTypes", item.statusColumnTypes).c_str());
		item.statusColumnWidths = wcsdup(kfrh.GetString(name, "statusColumnWidths", item.statusColumnWidths).c_str());

		item.columnTypes[0] = wcsdup(kfrh.GetString(name, "columnTypes4", item.columnTypes[0]).c_str());
		item.columnTypes[1] = wcsdup(kfrh.GetString(name, "columnTypes5", item.columnTypes[1]).c_str());

		item.columnWidths[0] = wcsdup(kfrh.GetString(name, "columnWidths4", item.columnWidths[0]).c_str());
		item.columnWidths[1] = wcsdup(kfrh.GetString(name, "columnWidths5", item.columnWidths[1]).c_str());

		GetFieldsFromConfig(item, kfrh, name, 0, "title4_");
		GetFieldsFromConfig(item, kfrh, name, 1, "title5_");
	}
}

void PluginCfg::SaveConfig(void) const
{
	LOG_INFO("=== Save config ===\n");

	auto path = INI_LOCATION;
	unlink(INI_LOCATION.c_str());

	KeyFileHelper kfh(INI_LOCATION);
	for( auto & [index, item] : def ) {
		const char * name = GetPanelName(index);
		kfh.SetString(name, "statusColumnTypes", item.statusColumnTypes);
		kfh.SetString(name, "statusColumnWidths", item.statusColumnWidths);
		kfh.SetString(name, "columnTypes4", item.columnTypes[0]);
		kfh.SetString(name, "columnTypes5", item.columnTypes[1]);
		kfh.SetString(name, "columnWidths4", item.columnWidths[0]);
		kfh.SetString(name, "columnWidths5", item.columnWidths[1]);

		std::string field_prefix("title4_");
		uint32_t offset = 0;
		for( auto & title: item.columnTitles[0] ) {
			if( title ) {
				std::string field = field_prefix + std::to_string(offset);
				kfh.SetString(name, field.c_str(), title);
			}
			offset++;
		}

		field_prefix = "title5_";
		offset = 0;
		for( auto & title: item.columnTitles[1] ) {
			std::string field = field_prefix + std::to_string(offset);
			if( title ) {
				kfh.SetString(name, field.c_str(), title);
				LOG_INFO("SetString(name: %s field %s title %S)\n", name, field.c_str(), title);
			} else {
				LOG_INFO("RemoveKey(name: %s field %s)\n", name, field.c_str());
				kfh.RemoveKey(name, field);
			}
			offset++;
		}
	}

	kfh.SetInt(INI_SECTION, "processAddToDisksMenu", processAddToDisksMenu);
	kfh.SetInt(INI_SECTION, "processAddToPluginsMenu", processAddToPluginsMenu);

	std::string _logfile(LOG_FILE);
	kfh.SetString(INI_SECTION, "logfile", _logfile);
	kfh.SetInt(INI_SECTION, "logEnable", logEnable);
	kfh.SetString(INI_SECTION, "prefix", prefix.c_str());
	kfh.Save();
}

PluginCfg::~PluginCfg()
{
	LOG_INFO("init %d\n", init);

	if( --init )
		return;

	LOG_INFO("=== FREE ===\n");

	for( auto & [index, item] : def ) {
		free((void *)item.statusColumnTypes);
		free((void *)item.statusColumnWidths);
		free((void *)item.columnTypes[0]);
		free((void *)item.columnTypes[1]);
		free((void *)item.columnWidths[0]);
		free((void *)item.columnWidths[1]);

		for( auto & title: item.columnTitles[0] ) {
			if( title )
				free((void *)title);
		}
		for( auto & title: item.columnTitles[1] ) {
			if( title )
				free((void *)title);
		}
	}
}

void PluginCfg::GetPluginInfo(struct PluginInfo *info)
{
	info->StructSize = sizeof(PluginInfo);
	info->Flags = 0;

	static const wchar_t *diskMenuStrings[2];
	static const wchar_t *pluginMenuStrings[2];
	static const wchar_t *pluginConfigStrings[1];

	diskMenuStrings[0] = GetMsg(MProcessDiskMenuString);

	info->DiskMenuStrings = diskMenuStrings;
	info->DiskMenuStringsNumber = (int)processAddToDisksMenu;

	pluginMenuStrings[0] = GetMsg(MProcessDiskMenuString);
	info->PluginMenuStrings = pluginMenuStrings;
	info->PluginMenuStringsNumber = (int)processAddToPluginsMenu;

	pluginConfigStrings[0] = GetMsg(MPluginConfigString);
	info->PluginConfigStrings = pluginConfigStrings;
	info->PluginConfigStringsNumber = ARRAYSIZE(pluginConfigStrings);

	info->CommandPrefix = prefix.c_str();
}

static const int DIALOG_WIDTH = 78;

enum {
	WinCfgCaptionIndex,
	WinCfgAddDiskMenuIndex,
	WinCfgAddPluginsMenuIndex,
	WinCfgSeparator1Index,
	WinCfgEanbleLogIndex,
	WinCfgEanbleLogStoreIndex,
	WinCfgEanbleLogEditIndex,
	WinCfgSeparator2Index,
	WinCfgConfigPrefixTextIndex,
	WinCfgConfigPrefixEditIndex,
	WinCfgConfigSaveSettingsCheckboxIndex,
	WinCfgSeparator3Index,
	WinCfgConfigWidePanelTextIndex,
	WinCfgConfigFullscreenPanelTextIndex,
	WinCfgConfigAviableWideFieldListBoxIndex,
	WinCfgConfigSelectedWideFieldListBoxIndex,
	WinCfgConfigAviableFullFieldListBoxIndex,
	WinCfgConfigSelectedFullFieldListBoxIndex,
	WinCfgMaxIndex
};

#define FIELDS_LIST_SIZE 8

LONG_PTR WINAPI CfgDialogProc(HANDLE hDlg, int msg, int param1, LONG_PTR param2)
{
	if( msg == DN_DRAWDLGITEM ) {
		if( param1 == WinCfgEanbleLogIndex ) {
			ShowHideElements(hDlg,
				WinCfgEanbleLogIndex,
				WinCfgEanbleLogStoreIndex,
				WinCfgEanbleLogEditIndex,
				WinCfgEanbleLogEditIndex);
		}
	}

	if( (msg == DN_KEY && (param2 == KEY_ENTER || param2 == KEY_SPACE)) || (msg == DN_MOUSECLICK) ) {
		switch( param1 ) {
		case WinCfgConfigAviableWideFieldListBoxIndex:
		case WinCfgConfigAviableFullFieldListBoxIndex:
			{
				FarListGetItem item = {(int)Plugin::psi.SendDlgMessage(hDlg,DM_LISTGETCURPOS,param1,0), 0};
				if( Plugin::psi.SendDlgMessage(
						hDlg,
						DM_LISTGETITEM,
						param1,
						(LONG_PTR)&item) )
					Plugin::psi.SendDlgMessage(
							hDlg,
							DM_LISTADDSTR,
							param1+1,
							(LONG_PTR)item.Item.Text);
			}
			break;
		case WinCfgConfigSelectedWideFieldListBoxIndex:
		case WinCfgConfigSelectedFullFieldListBoxIndex:
			{
				FarListDelete delitem = {(int)Plugin::psi.SendDlgMessage(hDlg,DM_LISTGETCURPOS,param1,0), 1};
				Plugin::psi.SendDlgMessage(
						hDlg,
						DM_LISTDELETE,
						param1,
						(LONG_PTR)&delitem);
			}
			break;
		};
	}

	return Plugin::psi.DefDlgProc(hDlg, msg, param1, param2);
}

void PluginCfg::FillFields(HANDLE hDlg, int listIndex, int index)
{
		auto & cfg = def[ProcessPanelIndex];
		const int total = static_cast<int>(sizeof(cfg.columnTitles[0])/sizeof(cfg.columnTitles[0][0]));

		assert( (size_t)index < sizeof(cfg.columnTitles)/sizeof(cfg.columnTitles[0]) );
		assert( listIndex < WinCfgMaxIndex );

		FarListGetItem li;
		li.ItemIndex = 0;
		std::wstring columnTypes;
		std::wstring columnWidths;

		while( Plugin::psi.SendDlgMessage(
						hDlg,
						DM_LISTGETITEM,
						listIndex,
						(LONG_PTR)&li) && \
						total > li.ItemIndex ) {
			LOG_INFO("%d. %S\n", li.ItemIndex, li.Item.Text);

			auto f = fields.find(std::wstring(li.Item.Text));

			if( f != fields.end() ) {
				if( !columnTypes.empty() ) {
					columnTypes += L",";
					columnWidths += L",";
				}
				columnTypes += f->second.fieldType;
				columnWidths += std::to_wstring(f->second.fieldWidth);

				LOG_INFO("cfg.columnTitles[index][li.ItemIndex] %p\n", cfg.columnTitles[index][li.ItemIndex]);
				free((void *)cfg.columnTitles[index][li.ItemIndex]);
				cfg.columnTitles[index][li.ItemIndex] = wcsdup(li.Item.Text);
			}
			li.ItemIndex++;
		}

		while( total > li.ItemIndex && cfg.columnTitles[index][li.ItemIndex] ) {
				free((void *)cfg.columnTitles[index][li.ItemIndex]);
				cfg.columnTitles[index][li.ItemIndex] = 0;
				li.ItemIndex++;
		}

		if( !columnTypes.empty() ) {
				free((void *)cfg.columnTypes[index]);
				cfg.columnTypes[index] = wcsdup(columnTypes.c_str());
				free((void *)cfg.columnWidths[index]);
				cfg.columnWidths[index] = wcsdup(columnWidths.c_str());
		}
}

int PluginCfg::Configure(int itemNumber)
{
	LOG_INFO("itemNumber = %d\n", itemNumber);
	bool change = false;

	static const DlgConstructorItem dci[] = {
		{DI_DOUBLEBOX,false,  DEFAUL_X_START, DIALOG_WIDTH-4, 0, {.lngIdstring = MConfigPluginSettings}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MAddDisksMenu}},
		{DI_CHECKBOX, false, 40,  0, 0, {.lngIdstring = MAddToPluginsMenu}},
		{DI_TEXT,     true,   5,  0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}},
		{DI_CHECKBOX, true,   5,  0, 0, {.lngIdstring = MEnableLog}},
		/* Store */{DI_CHECKBOX, false,  80,  0, DIF_HIDDEN, {0}},
		{DI_EDIT,     false, 21, DIALOG_WIDTH-6, 0, {0}},

		{DI_TEXT,     true,   5,  0, 0, {0}},
		{DI_TEXT,     true,   5,  0, 0, {.lngIdstring = ps_cfg_prefix}},
		{DI_EDIT,     false, 14, 35/*DIALOG_WIDTH-6*/, 0, {0}},

		{DI_CHECKBOX, false, 33,  0, 0, {.lngIdstring = MConfigSaveSettings}},

		{DI_TEXT,     true,   5,  0, DIF_BOXCOLOR|DIF_SEPARATOR, {0}},

		{DI_TEXT,     true,   12,  0, 0, {.ptrData = L"Wide panel:"}},
		{DI_TEXT,     false,   39,  0, 0, {.ptrData = L"Full screen panel:"}},

		{DI_LISTBOX,  true,   5,  20, DIF_LISTNOCLOSE, {.ptrData = L"Aviable:"}},
		{DI_LISTBOX,  false,  22, 35, DIF_LISTNOCLOSE, {.ptrData = L"Selected:"}},

		{DI_LISTBOX,  false,  37, 52, DIF_LISTNOCLOSE, {.ptrData = L"Aviable:"}},
		{DI_LISTBOX,  false,  54, 69, DIF_LISTNOCLOSE, {.ptrData = L"Selected:"}},

		{DI_ENDDIALOG, 0}
	};

	FarDlgConstructor fdc(&dci[0]);

	fdc.SetSelected(WinCfgAddDiskMenuIndex, processAddToDisksMenu);
	fdc.SetSelected(WinCfgAddPluginsMenuIndex, processAddToPluginsMenu);

	fdc.SetSelected(WinCfgEanbleLogIndex, logEnable);

	std::string _logfile(LOG_FILE);
	std::wstring logfile(_logfile.begin(), _logfile.end());
	fdc.SetText(WinCfgEanbleLogEditIndex, logfile.c_str());
	fdc.SetX(WinCfgEanbleLogEditIndex, wcslen(GetMsg(MEnableLog))+9);

	fdc.SetText(WinCfgConfigPrefixEditIndex, prefix.c_str());
	fdc.SetX(WinCfgConfigPrefixEditIndex, wcslen(GetMsg(ps_cfg_prefix))+6);

	fdc.SetX(WinCfgConfigSaveSettingsCheckboxIndex, DIALOG_WIDTH - 8 - wcslen(GetMsg(MConfigSaveSettings)));

	if( !logEnable )
		fdc.OrFlags(WinCfgEanbleLogEditIndex, DIF_DISABLE);


	std::vector<FarListItem> far_aviable_items;
	far_aviable_items.resize(fields.size());
	memset(&far_aviable_items.front(), 0, sizeof(FarListItem) * fields.size());
	int i = 0;
	for( auto & [name, cfg] : fields )
		far_aviable_items[i++].Text = name.c_str();
	far_aviable_items[0].Flags |= LIF_SELECTED;
	FarList far_aviable_list = {(int)far_aviable_items.size(), &far_aviable_items.front()};

	fdc.yEnd += FIELDS_LIST_SIZE;

	FarDialogItem * item = fdc.GetDialogItem(WinCfgConfigAviableWideFieldListBoxIndex);
	item->Y2 += FIELDS_LIST_SIZE;
	item->ListItems = &far_aviable_list;
	item = fdc.GetDialogItem(WinCfgConfigAviableFullFieldListBoxIndex);
	item->ListItems = &far_aviable_list;
	item->Y2 += FIELDS_LIST_SIZE;

	std::vector<FarListItem> far_selected_wide_items;
	auto & cfg = def[ProcessPanelIndex];
	auto titles = cfg.columnTitles[0];
	i = 0;
	while( titles[i] ) {
		FarListItem fli = {0, titles[i++], {0}};
		far_selected_wide_items.push_back(fli);
	}
	FarList far_selected_wide_list = {(int)far_selected_wide_items.size(), &far_selected_wide_items.front()};
	item = fdc.GetDialogItem(WinCfgConfigSelectedWideFieldListBoxIndex);
	item->ListItems = &far_selected_wide_list;
	item->Y2 += FIELDS_LIST_SIZE;
	
	std::vector<FarListItem> far_selected_full_items;
	titles = cfg.columnTitles[1];
	i = 0;
	while( titles[i] ) {
		FarListItem fli = {0, titles[i++], {0}};
		far_selected_full_items.push_back(fli);
	}
	FarList far_selected_full_list = {(int)far_selected_full_items.size(), &far_selected_full_items.front()};
	item = fdc.GetDialogItem(WinCfgConfigSelectedFullFieldListBoxIndex);
	item->ListItems = &far_selected_full_list;
	item->Y2 += FIELDS_LIST_SIZE;

	auto offSuffix = fdc.AppendOkCancel();

	fdc.SetDefaultButton(offSuffix + WinSuffixOkIndex);
	fdc.SetFocus(offSuffix + WinSuffixOkIndex);

	fdc.SetHelpTopic(L"Config");

	FarDialog dlg(&fdc, CfgDialogProc, (LONG_PTR)this);

	if( (dlg.Run() - offSuffix) == WinSuffixOkIndex ) {

		std::vector<ItemChange> chlst;
		change |= dlg.CreateChangeList(chlst);

		if( !change )
			return false;

		FillFields(dlg.GetDlg(), WinCfgConfigSelectedWideFieldListBoxIndex, 0);
		FillFields(dlg.GetDlg(), WinCfgConfigSelectedFullFieldListBoxIndex, 1);

		for( auto & item : chlst ) {
			switch(item.itemNum) {
			case WinCfgAddDiskMenuIndex:
				processAddToDisksMenu = bool(item.newVal.Selected);
				break;
			case WinCfgAddPluginsMenuIndex:
				processAddToPluginsMenu = bool(item.newVal.Selected);
				break;
			case WinCfgEanbleLogEditIndex:
				logfile = item.newVal.ptrData;
				if( logfile.size() < (LOG_MAX_PATH-1) && logfile.size() >= sizeof("/a") ) {
					_logfile = Wide2MB( logfile.c_str() );
					memmove(initial_log, _logfile.c_str(), _logfile.size()+1);
				}
				break;
			case WinCfgEanbleLogIndex:
				logEnable = bool(item.newVal.Selected);
				if( !logEnable )
					memmove(initial_log, "/dev/null", sizeof("/dev/null"));
				break;
			case WinCfgConfigPrefixEditIndex:
				prefix = item.newVal.ptrData;
				break;
			case WinCfgConfigSaveSettingsCheckboxIndex:
				SaveConfig();
				break;
			};
		}
	}
	return change;
}
