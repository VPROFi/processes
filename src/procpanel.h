#ifndef __PROCPANEL_H__
#define __PROCPANEL_H__

#include "farpanel.h"
#include "process/processes.h"

enum {
	ProcColumnPidIndex,
	ProcColumnPriorityIndex,
	ProcColumnNiceIndex,
	ProcColumnCpuIndex,
	ProcColumnMaxIndex
};

class ProcPanel : public FarPanel
{
private:

	std::unique_ptr<Processes> & processes;
	
	// copy and assignment not allowed
	ProcPanel(const ProcPanel&) = delete;
	void operator=(const ProcPanel&) = delete;

public:
	int ProcessKey(HANDLE hPlugin, int key, unsigned int controlState, bool & change) override;
	int GetFindData(struct PluginPanelItem **pPanelItem, int *pItemsNumber) override;
	void FreeFindData(struct PluginPanelItem * panelItem, int itemsNumber) override;
	int ProcessEvent(int event, void *param) override;
	void GetOpenPluginInfo(struct OpenPluginInfo * info) override;
	int DeleteFiles(struct PluginPanelItem *panelItem, int itemsNumber, int opMode) override;
	explicit ProcPanel(PanelIndex index, std::unique_ptr<Processes> & processes);
	virtual ~ProcPanel();
};


#endif // __PROCPANEL__
