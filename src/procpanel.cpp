#include "procpanel.h"
#include "fardialog.h"
#include "progress.h"
#include <common/log.h>
#include <common/utf8util.h>
#include <common/sizestr.h>
#include <utils.h>

extern const char * LOG_FILE;
#define LOG_SOURCE_FILE "procpanel.cpp"

ProcPanel::ProcPanel(PanelIndex index_, std::unique_ptr<Processes> & processes_):
	FarPanel(index_),
	processes(processes_)
{
	LOG_INFO("\n");
}

ProcPanel::~ProcPanel()
{
	LOG_INFO("\n");
}

int ProcPanel::ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change)
{
	LOG_INFO("\n");
	if( controlState == PKF_CONTROL && key == VK_F4) {
		change = true;
		Plugin::psi.Control(this, FCTL_SETSORTMODE, SM_NUMLINKS, 0);
		return TRUE;
	}

	if( controlState == PKF_CONTROL && (key == VK_F5 || key == VK_F8)) {
		//change = true;
		return FALSE;
	}

	if( controlState == 0 && key == VK_F3) {

		if( auto ppi = GetCurrentPanelItem() ) {
			if( Plugin::FSF.LStricmp(ppi->FindData.lpwszFileName, L"..") != 0 ) {
				CPUTimes ct = {0};
				GetCPUTimes(&ct);
				Process p((pid_t)ppi->FindData.ftLastWriteTime.dwLowDateTime, ct);
				p.Update();
				if( p.valid ) {
					std::string path = p.CreateProcessInfo();
					if( Plugin::psi.Editor(MB2Wide(path.c_str()).c_str(), ppi->FindData.lpwszFileName, 0, 0, -1, -1, EF_DISABLEHISTORY, 1, 1, CP_UTF8) == EEC_MODIFIED) {
					}
					unlink(path.c_str());
				} else {
					std::wstring not_found;
					not_found += L"[" + std::to_wstring(ppi->FindData.ftLastWriteTime.dwLowDateTime) + L"] ";
					not_found += ppi->FindData.lpwszFileName;
					not_found += L" ... not found";
					const wchar_t* err_msg[] = {GetMsg(ps_title_short), not_found.c_str()};
					Plugin::psi.Message(Plugin::psi.ModuleNumber, FMSG_WARNING | FMSG_MB_OK, nullptr, err_msg, sizeof(err_msg) / sizeof(err_msg[0]), 0);
				}
			}
			FreePanelItem(ppi);
		}
		return TRUE;
	}

	return IsPanelProcessKey(key, controlState);
}

#ifndef SET_FILETIME
#define SET_FILETIME(ft, v64) \
   (ft)->dwLowDateTime = (DWORD)v64; \
   (ft)->dwHighDateTime = (DWORD)(v64 >> 32);
#endif

int ProcPanel::GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber, int opMode)
{
	LOG_INFO("\n");

	*pItemsNumber = processes->procs.size();
	*pPanelItem = (struct PluginPanelItem *)malloc((*pItemsNumber) * sizeof(PluginPanelItem));
	memset(*pPanelItem, 0, (*pItemsNumber) * sizeof(PluginPanelItem));
	PluginPanelItem * pi = *pPanelItem;

	for( const auto & [pid, proc]: processes->procs ) {
		pi->FindData.lpwszFileName = wcsdup(MB2Wide(proc->name.c_str()).c_str());
		pi->FindData.dwFileAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

		SET_FILETIME(&pi->FindData.ftCreationTime, proc->startTimeMs);
		SET_FILETIME(&pi->FindData.ftLastWriteTime, (uint64_t)proc->pid);

		if( proc->flags & PF_KTHREAD )
			pi->FindData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

		pi->FindData.nFileSize = ((uint64_t)proc->m_resident)*proc->pageSize;
		pi->NumberOfLinks = (DWORD)(proc->percent_cpu * 100.0);

		pi->Owner = wcsdup(MB2Wide(proc->user.c_str()).c_str());
		pi->Group = wcsdup(MB2Wide(proc->group.c_str()).c_str());

		const wchar_t ** customColumnData = (const wchar_t **)malloc(ProcColumnMaxIndex*sizeof(const wchar_t *));
		if( customColumnData ) {
			memset(customColumnData, 0, ProcColumnMaxIndex*sizeof(const wchar_t *));
			customColumnData[ProcColumnPidIndex] = DublicateCountString(proc->pid);
			customColumnData[ProcColumnPriorityIndex] = DublicateCountString(proc->priority);
			customColumnData[ProcColumnNiceIndex] = DublicateCountString(proc->nice);
			customColumnData[ProcColumnCpuIndex] = DublicateFloatPercentString(proc->percent_cpu);
			customColumnData[ProcColumnTimeIndex] = wcsdup(MB2Wide(msec_to_str(GetRealtimeMs() - proc->startTimeMs)).c_str());
			pi->CustomColumnNumber = ProcColumnMaxIndex;
			pi->CustomColumnData = customColumnData;
		}
		pi++;
	}
	return int(true);
}

void ProcPanel::FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber_)
{
	LOG_INFO("\n");
	auto itemsNumber = itemsNumber_;
	while( itemsNumber-- ) {
		assert( (panelItem+itemsNumber)->FindData.dwFileAttributes & FILE_FLAG_DELETE_ON_CLOSE );
		free((void *)(panelItem+itemsNumber)->FindData.lpwszFileName);
	}
	FarPanel::FreeFindData(panelItem, itemsNumber_);
}

int ProcPanel::ProcessEvent(int event, void *param)
{
	LOG_INFO("\n");
	if( event != FE_IDLE )
		return int(false);

	processes->Update();

	Plugin::psi.Control(this, FCTL_UPDATEPANEL, TRUE, 0);
	Plugin::psi.Control(this, FCTL_REDRAWPANEL, 0, 0);

	return int(true);	
}

void ProcPanel::GetOpenPluginInfo(struct OpenPluginInfo * info)
{
	FarPanel::GetOpenPluginInfo(info);
	static wchar_t panelName[sizeof("Process list: 100000.00%       ")];
	if( Plugin::FSF.snprintf(panelName, ARRAYSIZE(panelName), L"%S CPU(%.02f%%)", GetMsg(MPanelProcessTitle), processes->total_percent) > 0 )
		info->PanelTitle = panelName;
}

int ProcPanel::DeleteFiles(struct PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
	LOG_INFO("\n");
	if( itemsNumber == 1 && panelItem->FindData.lpwszFileName && Plugin::FSF.LStricmp(panelItem->FindData.lpwszFileName, L"..") == 0 )
		return int(false);


	PluginPanelItem * pi = panelItem;
	auto _itemsNumber = itemsNumber;

	std::vector<std::wstring> items;
	std::vector<const wchar_t *> msg;
	items.push_back(L"Kill processes: ");
	msg.push_back(items.back().c_str());

	while( _itemsNumber-- ) {
		std::wstring m;
		m += L"[";
		m += pi->CustomColumnData[ProcColumnPidIndex];
		m += L"] ";
		m += pi->FindData.lpwszFileName;
		items.push_back(m);
		msg.push_back(items.back().c_str());
		pi++;
	}

	if( Plugin::psi.Message(Plugin::psi.ModuleNumber, FMSG_MB_YESNO | FMSG_WARNING, nullptr, &msg.front(), msg.size(), 0) != 0)
		return int(false);

	pi = panelItem;
	while( itemsNumber-- ) {
		Process proc(pi->FindData.ftLastWriteTime.dwLowDateTime);
		//Process proc(Plugin::FSF.atoi(pi->CustomColumnData[ProcColumnPidIndex]));
		proc.Kill();
		pi++;
	}

	Plugin::psi.Control(this, FCTL_UPDATEPANEL, TRUE, 0);
	Plugin::psi.Control(this, FCTL_REDRAWPANEL, 0, 0);

	return int(true);
}

int ProcPanel::GetFiles(struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode)
{
	LOG_INFO("\n");
	if( !(opMode & (OPM_FIND | OPM_SILENT | OPM_VIEW | OPM_QUICKVIEW) ) )
		return int(false);

	while( itemsNumber-- ) {
		if( *destPath && !(panelItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ) {
			LOG_INFO("panelItem->FindData.lpwszFileName %S *destPath: %S\n", panelItem->FindData.lpwszFileName, *destPath);
			CPUTimes ct = {0};
			GetCPUTimes(&ct);
			Process p((pid_t)panelItem->FindData.ftLastWriteTime.dwLowDateTime, ct);
			p.Update();
			if( p.valid ) {
				std::string path(Wide2MB(*destPath));
				path += "/";
				path += Wide2MB(panelItem->FindData.lpwszFileName);
				p.CreateProcessInfo(path.c_str());
			}
		}
		panelItem++;
		destPath++;
	}
	return int(true);
}

int ProcPanel::SetDirectory(const wchar_t *dir, int opMode)
{
	LOG_INFO("\n");
	return int(true);
}
