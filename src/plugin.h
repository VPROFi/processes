#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "plugincfg.h"
#include "farpanel.h"

class Plugin {
	private:

		// copy and assignment not allowed
		Plugin(const Plugin&) = delete;
		void operator=(const Plugin&) = delete;

	protected:

		// Configuration
		PluginCfg * cfg;

		// config change
		bool config_change;

		// Panels
		std::vector<std::unique_ptr<FarPanel>> panel;

	public:

		explicit Plugin(const PluginStartupInfo * info);
		virtual ~Plugin();

		// far2l backconnect
		static struct PluginStartupInfo psi;
		static struct FarStandardFunctions FSF;

		// First call
		virtual HANDLE OpenFilePlugin(const wchar_t * name,const unsigned char * data, int dataSize, int opMode) = 0;
		virtual HANDLE OpenPlugin(int openFrom, INT_PTR item) = 0;
		virtual void ClosePlugin(HANDLE hPlugin) = 0;
	
		// far2l api
		virtual int GetFindData(HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int opMode);
		virtual void FreeFindData(HANDLE hPlugin, struct PluginPanelItem * PanelItem, int ItemsNumber);
		virtual void GetPluginInfo(struct PluginInfo *info);
		virtual void GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *info);
		virtual int SetDirectory(HANDLE hPlugin, const wchar_t *dir, int opMode);
		virtual int SetFindList(HANDLE hPlugin, const struct PluginPanelItem *panelItem, int itemsNumber);
		virtual int DeleteFiles(HANDLE hPlugin, struct PluginPanelItem *panelItem, int itemsNumber, int opMode);
		virtual int ProcessKey(HANDLE hPlugin,int key,unsigned int controlState);
		virtual int ProcessEvent(HANDLE hPlugin,int event,void *param);
		virtual int GetFiles(HANDLE hPlugin,struct PluginPanelItem *panelItem,int itemsNumber,int move, const wchar_t ** destPath, int opMode);
		virtual int Configure(int itemNumber);
};

#endif /* __PLUGIN_H__ */
