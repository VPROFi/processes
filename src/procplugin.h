#ifndef __PROCPLUGIN_H__
#define __PROCPLUGIN_H__

#include "plugin.h"
#include "process/processes.h"

struct PluginUserData {
	DWORD size;
	union {
		void * data;
		Process * proc;
	} data;
};

class ProcPlugin : public Plugin {
	private:
		// Panels
		enum {
			PanelProcesses,
			PanelTypeMax
		};

		std::unique_ptr<Processes> processes;

		// copy and assignment not allowed
		ProcPlugin(const ProcPlugin&) = delete;
		void operator=(const ProcPlugin&) = delete;

	public:

		explicit ProcPlugin(const PluginStartupInfo * info);
		virtual ~ProcPlugin() override;

		// far2l api
		HANDLE OpenFilePlugin(const wchar_t * name,const unsigned char * data, int dataSize, int opMode) override;
		int GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber, int opMode) override;
		HANDLE OpenPlugin(int openFrom, INT_PTR item) override;
		void ClosePlugin(HANDLE hPlugin) override;
};

#endif /* __PROCPLUGIN_H__ */
