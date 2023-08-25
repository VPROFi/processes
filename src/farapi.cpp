#include "farapi.h"
#include "plugin.h"
#include <string>
#include <cassert>
#include <utils.h>
#include <common/log.h>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "farapi.cpp"

FarApi::FarApi(struct PluginStartupInfo & _psi, struct FarStandardFunctions & _FSF):
	psi(_psi),
	FSF(_FSF)
{
	LOG_INFO("\n");
}

FarApi::FarApi():
	psi(Plugin::psi),
	FSF(Plugin::FSF)
{
	LOG_INFO("\n");
}


FarApi::~FarApi()
{
	LOG_INFO("\n");
}

void FarApi::GetPanelInfo(PanelInfo & pi) const
{
	psi.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
}


PluginPanelItem * FarApi::GetPanelItem(intptr_t itemNum) const
{
	auto size = psi.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, itemNum, 0);
	PluginPanelItem * ppi = (PluginPanelItem *)malloc(size);
	if( ppi != 0 )
		psi.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, itemNum, (LONG_PTR)ppi);
	return ppi;
}

PluginPanelItem * FarApi::GetCurrentPanelItem(PanelInfo * piret) const
{
	struct PanelInfo pi = {0};
	psi.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if( piret )
		*piret = pi;
	return GetPanelItem(pi.CurrentItem);
}

PluginPanelItem * FarApi::GetSelectedPanelItem(intptr_t selectedItemNum) const
{
	auto size = psi.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, selectedItemNum, 0);
	PluginPanelItem * ppi = (PluginPanelItem *)malloc(size);
	if( ppi != 0 )
		psi.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, selectedItemNum, (LONG_PTR)ppi);
	return ppi;
}

void FarApi::FreePanelItem(PluginPanelItem * ppi) const
{
	free((void *)ppi);
}

const wchar_t * FarApi::GetMsg(int msgId) const
{
	assert( psi.GetMsg != 0 );
	return psi.GetMsg(Plugin::psi.ModuleNumber, msgId);
}

int FarApi::Select(HANDLE hDlg, const wchar_t ** elements, int count, uint32_t setIndex) const
{
	int index = 0;
	auto menuElements = std::make_unique<FarMenuItem[]>(count);
	memset(menuElements.get(), 0, sizeof(FarMenuItem)*count);
	do {
		menuElements[index].Text = elements[index];
	} while( ++index < count );

	menuElements[0].Selected = 1;

	index = psi.Menu(Plugin::psi.ModuleNumber, -1, -1, 0, FMENU_WRAPMODE|FMENU_AUTOHIGHLIGHT, \
			 L"Select:", 0, L"", nullptr, nullptr, menuElements.get(), count);
	if( index >= 0 && index < count )
		psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, setIndex, (LONG_PTR)menuElements[index].Text);
	return index;
}

int FarApi::SelectNum(HANDLE hDlg, const wchar_t ** elements, int count, const wchar_t * subTitle, uint32_t setIndex) const
{
	auto index = Select(hDlg, elements, count, setIndex);
	if( index == (count-1) ) {
		static wchar_t num[sizeof("4294967296")] = {0};
		psi.InputBox(L"Enter number:", subTitle, 0, 0, num, ARRAYSIZE(num), 0, FIB_NOUSELASTHISTORY);
		psi.SendDlgMessage(hDlg, DM_SETTEXTPTR, setIndex, (LONG_PTR)num);
		psi.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
	}
	return index;
}

std::wstring FarApi::towstr(const char * name) const
{
	std::string _s(name);
	return std::wstring(_s.begin(), _s.end());
}

std::string FarApi::tostr(const wchar_t * name) const
{
	std::wstring _s(name);
	return std::string(_s.begin(), _s.end());
}

const wchar_t * FarApi::DublicateCountString(int64_t value) const
{
	wchar_t max64[sizeof("18446744073709551615")] = {0};

#if INTPTR_MAX == INT32_MAX
	if( FSF.snprintf(max64, ARRAYSIZE(max64), L"%lld", value) > 0 )
#elif INTPTR_MAX == INT64_MAX
	if( FSF.snprintf(max64, ARRAYSIZE(max64), L"%ld", value) > 0 )
#else
    #error "Environment not 32 or 64-bit."
#endif
		return wcsdup(max64);

	return wcsdup(L"");
}

const wchar_t * FarApi::DublicateFileSizeString(uint64_t value) const
{
	if( value > 100 * 1024 ) {
		std::wstring _tmp = FileSizeString(value);
		return wcsdup(_tmp.c_str());
	}
	return DublicateCountString((int64_t)value);
}

const wchar_t * FarApi::DublicateFloatPercentString(float value) const
{
	wchar_t max64[sizeof("186744073709551615.18%")] = {0};
	if( FSF.snprintf(max64, ARRAYSIZE(max64), L"%.02f%%", value) > 0 )
		return wcsdup(max64);
	return wcsdup(L"");
}