#include "processes.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/sizestr.h>

#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <set>

#ifndef MAIN_PROCESSES
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

#define LOG_SOURCE_FILE "processes.cpp"

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

Processes::Processes()
{
	LOG_INFO("\n");
	memset(&ct, 0, sizeof(ct));
	Update();
};


bool Processes::Update(void)
{
	LOG_INFO("\n");

	GetCPUTimes(&ct);

	auto proc = opendir("/proc");
	if( !proc ) {
		LOG_ERROR("opendir(\"/proc\") ... error (%s)\n", errorname(errno));
		return false;
	}

	total_percent = 0.0F;

	struct dirent * entry;
	std::set<pid_t> valid_pid;
	while( (entry = readdir(proc)) ) {
		if( entry->d_type != DT_DIR )
			continue;
		pid_t pid;
		if( !(pid = strtoul(entry->d_name, NULL, 10)) )
			continue;

		valid_pid.insert(pid);

		if( procs.find(pid) == procs.end() )
			procs[pid] = std::make_unique<Process>(pid, ct);
		else
			procs[pid]->Update();

		total_percent += procs[pid]->percent_cpu;
	}
	closedir(proc);

	std::set<pid_t> invalid_pid;

	for( const auto& [id, ps] : procs )
		if( valid_pid.find(id) == valid_pid.end() )
			invalid_pid.insert(id);

	for( const auto _pid : invalid_pid ) {
		LOG_INFO("delete process %u\n", _pid);
		procs.extract( _pid );
	}

	LOG_INFO("total_percent %f%%\n", total_percent);

	return true;
}

Processes::~Processes()
{
	LOG_INFO("\n");
};


void Processes::Log(void)
{

}

#ifdef MAIN_PROCESSES

int RootExec(const char * cmd, int flags)
{
	std::string _cmd("sudo /bin/sh -c \'");
	_cmd += cmd;
	_cmd += "'";
	LOG_INFO("Try exec \"%s\"\n", _cmd.c_str());
	return system(_cmd.c_str());
}

int Exec(const char * cmd, int flags)
{
	std::string _cmd("/bin/sh -c \'");
	_cmd += cmd;
	_cmd += "'";
	LOG_INFO("Try exec \"%s\"\n", _cmd.c_str());
	return system(_cmd.c_str());
}

int main(int argc, char * argv[])
{
	Processes procs = Processes();
	procs.Update();
	procs.Log();
	LOG_INFO("============================================================\n");
	sleep(3);
	procs.Update();
	LOG_INFO("============================================================\n");
	sleep(3);
	procs.Update();
	return 0;
}
#endif //MAIN_PROCESSES
