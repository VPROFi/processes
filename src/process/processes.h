#ifndef __PROCESSES_H__
#define __PROCESSES_H__

#include "process.h"
#include <map>
#include <memory>

struct Processes {
	std::map<pid_t, std::unique_ptr<Process>> procs;

	typedef std::map<pid_t, std::unique_ptr<Process>>::const_iterator const_iterator;
	const_iterator begin() const { return procs.begin(); };
	const_iterator end() const { return procs.end(); };
	const_iterator find(pid_t pid) const { return procs.find(pid); };
	int size(void) const { return procs.size(); };

	CPUTimes ct;

	float total_percent;

	bool UpdateTcpUdp(const char * path);
	bool UpdateNet(void);
	bool Update(void);
	void Log(void);
	Processes();
	~Processes();
};

#endif // __PROCESSES_H__