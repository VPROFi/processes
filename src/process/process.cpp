#include "process.h"
#include "sockets.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/sizestr.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/sysmacros.h>
#include <pwd.h>
#include <grp.h>

#include <cstring>
#include <memory>

enum EXECUTEFLAGS
{
	EF_HIDEOUT = 0x01,    // dont display output of the command
	EF_NOWAIT = 0x02,     // dont wait for command completion
	EF_SUDO = 0x04,       // command must be run with root priviledges
	EF_NOTIFY = 0x08,     // notify when command completed (if such notifications enabled in settings)
	EF_NOCMDPRINT = 0x10, // dont print command in command line nor include it to history
	EF_OPEN = 0x20        // use desktop shell (if present) to open command (e.g. URLs, documents..)
};

#ifndef MAIN_PROCESS
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

#define LOG_SOURCE_FILE "process.cpp"

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

void Process::FreeFileData(char * buf) const
{
	::FreeFileData(buf);
}

char * Process::GetFileData(const char * fmt, pid_t _ppid, pid_t _tpid, ssize_t *readed) const
{
	char path[sizeof("/proc/4294967296/task/4294967296/coredump_filter")];
	if( snprintf(path, sizeof(path), fmt, _ppid, _tpid) < 0 ) {
		LOG_ERROR("snprintf(fmt=\"%s\",_ppid=%u, _tpid=%u) ... error (%s)\n", fmt, _ppid, _tpid, errorname(errno));
		return 0;
	}

	return ::GetFileData(path, readed);
}

static uint64_t GetHZ(void)
{
	static int hz = 0;

	if( !hz )
		hz = sysconf(_SC_CLK_TCK);

	if( !hz )
		hz = 100;

	return hz;
}

void Process::Update()
{
	/* fill in default values for older kernels */
	processor = 0;
	rtprio = -1;
	sched = -1;
	nlwp = 0;

	std::string path("/proc/");
	path += std::to_string(pid);
	struct stat sbuf;
	if( stat(path.c_str(), &sbuf) == 0 ) {
		LOG_INFO("User ID of owner: %d\n", sbuf.st_uid);
		LOG_INFO("Group ID of owner: %d\n", sbuf.st_gid);
		const struct passwd* userData = getpwuid(sbuf.st_uid);
		if( userData && userData->pw_name ) {
			user = userData->pw_name;
			LOG_INFO("userData.pw_name: %s\n", userData->pw_name);
		}
	    struct group *gr = getgrgid(sbuf.st_gid);
		if( gr && gr->gr_name ) {
			group = gr->gr_name;
			LOG_INFO("gr->gr_name: %s\n", gr->gr_name);
		}
	} else {
		LOG_ERROR("stat(\"%s\") ... error (%s)\n", path.c_str(), errorname(errno));
	}

	char * buf = GetFileData("/proc/%u/stat", pid, 0, 0);
	if( !buf )
		return;

	valid = false;
	do {
		char *s = strchr(buf, '(')+1;
		if( !s ) break;
		char *s2 = strrchr(s, ')');
		if( !s2 || !s2[1]) break;
		name = std::string(s,strrchr(s, ')'));

		s = s2 + 2; // skip ") "

		if( sscanf(s,
			"%c "                      // state
			"%d %d %d %d %d "          // ppid, pgrp, sid, tty_nr, tty_pgrp
			"%lu %lu %lu %lu %lu "     // flags, min_flt, cmin_flt, maj_flt, cmaj_flt
			"%llu %llu %llu %llu "     // utime, stime, cutime, cstime
			"%d %d "                   // priority, nice
			"%d "                      // num_threads
			"%lu "                     // 'alarm' == it_real_value (obsolete, always 0)
			"%llu "                    // start_time
			"%lu "                     // vsize
			"%lu "                     // rss
			"%lu %lu %lu %lu %lu %lu " // rsslim, start_code, end_code, start_stack, esp, eip
			"%*s %*s %*s %*s "         // pending, blocked, sigign, sigcatch                      <=== DISCARDED
			"%lu %*u %*u "             // 0 (former wchan), 0, 0                                  <=== Placeholders only
			"%d %d "                   // exit_signal, task_cpu
			"%d %d "                   // rt_priority, policy (sched)
			"%llu %llu %llu",          // blkio_ticks, gtime, cgtime
			&state,
			&ppid, &pgrp, &session, &tty, &tpgid,
			&flags, &min_flt, &cmin_flt, &maj_flt, &cmaj_flt,
			&utime, &stime, &cutime, &cstime,
			&priority, &nice,
			&nlwp,
			&alarm,
			&start_time,
			&vsize,
			&rss,
			&rss_rlim, &start_code, &end_code, &start_stack, &kstk_esp, &kstk_eip,
			/*     signal, blocked, sigignore, sigcatch,   */ /* can't use */
			&wchan, /* &nswap, &cnswap, */  /* nswap and cnswap dead for 2.4.xx and up */
			/* -- Linux 2.0.35 ends here -- */
			&exit_signal, &processor,  /* 2.2.1 ends with "exit_signal" */
			/* -- Linux 2.2.8 to 2.5.17 end here -- */
			&rtprio, &sched,  /* both added to 2.5.18 */
			&blkio_tics, &gtime, &cgtime) < 0 ) {
			LOG_ERROR("sscanf(%s) ... error (%s)\n", buf, errorname(errno));
			break;
		}

		if( flags & PF_KTHREAD ) {
			name = "["+name+"]";
		}

		if(	!ct.total_period )
			percent_cpu = 0.0F;
		else {
			percent_cpu = (ct.period < 1E-6) ? 0.0F : ((((double)(utime + stime - lasttimes)) / ct.total_period) * 100.0);
			percent_cpu = CLAMP(percent_cpu, 0.0F, ct.activeCPUs * 100.0F);
			LOG_INFO("buf: %s\n", buf);
			LOG_INFO("utime + stime: %ld\n", utime + stime);
			LOG_INFO("lasttimes:     %ld\n", lasttimes);
			LOG_INFO("percent_cpu: %.10f%%\n", percent_cpu);
		}

		lasttimes = utime + stime;
		startTimeMs = ct.btime*1000 + start_time*1000/GetHZ();

		valid = true;

	} while(0);

	nlwp = 1;

	FreeFileData(buf);

	buf = GetFileData("/proc/%u/statm", pid, 0, 0);
	if( !buf )
		return;

	long int dummy, dummy2;

	LOG_INFO("buf: %s\n", buf);

	sscanf(buf, "%ld %ld %ld %ld %ld %ld %ld",
                  &m_virt,
                  &m_resident,
                  &m_share,
                  &m_trs,
                  &dummy, /* unused since Linux 2.6; always 0 */
                  &m_drs,
                  &dummy2); /* unused since Linux 2.6; always 0 */

	FreeFileData(buf);

	Log();

	return;
}

std::string Process::GetTempName(void) const
{
	char path[sizeof("/tmp/processXXXXXX")];
	memmove(path, "/tmp/processXXXXXX", sizeof("/tmp/processXXXXXX"));
	int f = mkstemp(&path[0]);
	fchmod(f, 0666);
	close(f);
	return std::string(path);
}

char * Process::GetCommandOutput(std::string & cmd, ssize_t *readed) const
{
	std::string path(std::move(GetTempName()));
	char * buf = nullptr;

	cmd += " 2>/dev/null >>";
	cmd += path;

	if( Exec(cmd.c_str(), EF_NOCMDPRINT | EF_HIDEOUT) == 0 )
		buf = ::GetFileData(path.c_str(), readed);

	// /tmp dir use (sticky bit), remove file before use root
	unlink(path.c_str());

	if( buf )
		return buf;

	if( RootExec(cmd.c_str(), EF_NOCMDPRINT | EF_HIDEOUT) == 0 )
		buf = ::GetFileData(path.c_str(), readed);

	path = "rm " + path;
	RootExec(path.c_str(), EF_NOCMDPRINT | EF_HIDEOUT);
	return buf;
}

char * Process::GetProcInfo(const char * info, ssize_t *readed) const
{
	std::string cmd("cat ");
	cmd += "/proc/";
	cmd += std::to_string(pid) + "/";
	cmd += info;
	return GetCommandOutput(cmd, readed);
}

char * Process::GetFilesInfo(ssize_t *readed) const
{
	std::string cmd("ls -l ");
	cmd += "/proc/";
	cmd += std::to_string(pid) + "/fd";
	return GetCommandOutput(cmd, readed);
}

std::string Process::GetProcInfoToString(const char * field, const char * separator) const
{
	std::string res;
	ssize_t readed = 0;
	char * buf = GetProcInfo(field, &readed);
	if( buf ) {
		auto ptr = buf;
		while( readed > 0 ) {
			ssize_t size = strlen(ptr)+1;
			assert( readed >= size );
			if( *ptr ) {
				if( !res.empty() )
					res += separator;
				res += ptr;
			}
			ptr += size;
			readed -= size;
		}
		FreeFileData(buf);
	}
	return res;
}

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 1024
#endif

void Process::CapabilitiesToString(std::string & str, uint64_t cap) const
{
	#define __CAP_BITS   38
	#define __CAPS_ALL   ((1ll<<__CAP_BITS)-1)
	static const char * cap_names[__CAP_BITS] = {
	      /* 0 */	"cap_chown",
	      /* 1 */	"cap_dac_override",
	      /* 2 */	"cap_dac_read_search",
	      /* 3 */	"cap_fowner",
	      /* 4 */	"cap_fsetid",
	      /* 5 */	"cap_kill",
	      /* 6 */	"cap_setgid",
	      /* 7 */	"cap_setuid",
	      /* 8 */	"cap_setpcap",
	      /* 9 */	"cap_linux_immutable",
	      /* 10 */	"cap_net_bind_service",
	      /* 11 */	"cap_net_broadcast",
	      /* 12 */	"cap_net_admin",
	      /* 13 */	"cap_net_raw",
	      /* 14 */	"cap_ipc_lock",
	      /* 15 */	"cap_ipc_owner",
	      /* 16 */	"cap_sys_module",
	      /* 17 */	"cap_sys_rawio",
	      /* 18 */	"cap_sys_chroot",
	      /* 19 */	"cap_sys_ptrace",
	      /* 20 */	"cap_sys_pacct",
	      /* 21 */	"cap_sys_admin",
	      /* 22 */	"cap_sys_boot",
	      /* 23 */	"cap_sys_nice",
	      /* 24 */	"cap_sys_resource",
	      /* 25 */	"cap_sys_time",
	      /* 26 */	"cap_sys_tty_config",
	      /* 27 */	"cap_mknod",
	      /* 28 */	"cap_lease",
	      /* 29 */	"cap_audit_write",
	      /* 30 */	"cap_audit_control",
	      /* 31 */	"cap_setfcap",
	      /* 32 */	"cap_mac_override",
	      /* 33 */	"cap_mac_admin",
	      /* 34 */	"cap_syslog",
	      /* 35 */	"cap_wake_alarm",
	      /* 36 */	"cap_block_suspend",
	      /* 37 */	"cap_audit_read"
	};

	cap = cap & __CAPS_ALL;

	if( cap == __CAPS_ALL ) {
		str += "full";
		return;
	}

	size_t offset = 0;
	while( cap && offset < sizeof(cap_names)/sizeof(cap_names[0]) ) {
		if( cap & 1 ) {
			str += cap_names[offset];
			if( cap >> 1 )
			str += ", ";
		}
		cap = cap >> 1;
		offset += 1;
	};
}

std::string Process::GetProcStatus(void) const
{
	std::string status;

	char path[sizeof("/proc/4294967296/status")];
	if( snprintf(path, sizeof(path), "/proc/%u/status", pid) < 0 ) {
		LOG_ERROR("snprintf(fmt=\"/proc/%u/status\") ... error (%s)\n", pid, errorname(errno));
		return status;
	}

	auto fs = fopen(path, "r");
	if( !fs ) {
		LOG_ERROR("fopen(\"%s\") ... error (%s)\n", path, errorname(errno));
		return status;
	}

	auto buf = std::make_unique<char[]>(MAX_LINE_LENGTH+1);
	char * ptr = buf.get();
	size_t size = MAX_LINE_LENGTH+1;

	uint64_t cap = 0;

	while( fgets(ptr, size, fs) ) {
		if( memcmp(ptr, "Cap", 3) != 0 )
			continue;
		if( memcmp(ptr, "CapInh:\t", 8) == 0 || \
		    memcmp(ptr, "CapPrm:\t", 8) == 0 || \
		    memcmp(ptr, "CapEff:\t", 8) == 0 || \
		    memcmp(ptr, "CapBnd:\t", 8) == 0 ) {
			cap = strtoull(&ptr[8], 0, 16);
			ptr[strlen(ptr)-2] = 0;
			status += ptr;
			if( cap ) {
				status += " ";
				CapabilitiesToString(status, cap);
			}
			status += "\n";
		}
	}
	fclose(fs);
	return status;
}

std::string Process::CreateProcessInfo(const char * filePath)
{
	auto skts = std::make_unique<Sockets>();
	if( skts != nullptr )
		skts->Update();

	std::string path;

	if( filePath )
		path = std::move(filePath);
	else
		path = std::move(GetTempName());

	FILE* file = fopen(path.c_str(), "a");
	if( !file ) {
		LOG_ERROR("fopen(\"%s\", \"r\") ... error (%s)\n", path.c_str(), errorname(errno));
		return std::string();
	}

	LOG_INFO("path: %s\n", path.c_str());

	fprintf(file, "pid:             | %d\nname:            | %s\nuser/group:      | %s:%s\nstate:           | %c\n", pid, name.c_str(), user.c_str(), group.c_str(), state);
	fprintf(file, "virtual memory:  | %s\n", size_to_str((unsigned long long)m_virt*pageSize));
	fprintf(file, "resident memory: | %s\n", size_to_str((unsigned long long)m_resident*pageSize));
	if( startTimeMs )
		fprintf(file, "start_time:      | %s\n", msec_to_str(GetRealtimeMs() - startTimeMs));
	fprintf(file, "other:           | ppid: %d, pgrp: %d, session: %d, tty: %d, tpgid: %d priority: %d, nice: %d\n", ppid, pgrp, session, tty, tpgid, priority, nice);
	//fprintf(file, "flags: 0x%08lX, min_flt: %ld, cmin_flt: %ld, maj_flt: %ld, cmaj_flt: %ld\n", flags, min_flt, cmin_flt, maj_flt, cmaj_flt);
	//fprintf(file, "utime %llu, stime %llu, cutime %llu, cstime %llu\n", utime, stime, cutime, cstime);


	std::string env = GetProcInfoToString("environ", "\n");
	std::string cmdline = GetProcInfoToString("cmdline", " ");
	std::string sockets;
	std::string net;
	std::string files;
	std::string maps;
	std::string status = GetProcStatus();

	ssize_t readed = 0;
	char * buf = GetProcInfo("maps", &readed);
	if( buf ) {
		auto ptr = buf;
		ssize_t num = 0;
		while( num < readed ) {
			if( buf[num] == 0x0A )
				buf[num] = 0;
			num++;
		}

		while( readed > 0 ) {

			ssize_t  size = strlen(ptr)+1;
			assert( readed >= size );

			if( size > 49 && (ptr[20] == 'x' || ptr[28] == 'x' || ptr[24] == 'x' || ptr[36] == 'x' || (size > 73 && ptr[73] == '[') || ptr[49] == '[' ) ) {
				maps += ptr;
				maps += "\n";
			}
			ptr += size;
			readed -= size;
		}

		FreeFileData(buf);		
	}

	buf = GetFilesInfo(&readed);
	if( buf ) {

		ssize_t num = 0;
		while( num < readed ) {
			if( buf[num] == 0x0A )
				buf[num] = 0;
			num++;
		}

		auto ptr = buf;

		while( readed > 0 ) {

			ssize_t size = strlen(ptr)+1;
			assert( readed >= size );

			auto lnk = strstr(ptr, " -> ");
			if( lnk ) {
				auto fd = lnk;
				lnk += 4;
				fd[0] = '\0';

				while( fd > ptr && *fd != ' ' )
					fd--;

				if( fd++ > ptr ) {
					if( *lnk == '/' || skts == nullptr || memcmp(lnk, "socket:[", sizeof("socket:[")-1) != 0 ) {
						files += fd;
						files += " -> ";
						files += lnk;
						files += "\n";
					} else {
						sockets += fd;
						sockets += " -> ";
						lnk += sizeof("socket:[") - 1;
						lnk[strlen(lnk)-1] = '\0';
						unsigned long long inode = std::stoull(lnk);
						auto it = skts->find(inode);
						if( it != skts->end() ) {
							sockets += it->second;
							if( strncmp(it->second.c_str(), "unix:", 5) != 0 )
								net += it->second + " socket:[" + std::to_string(inode) + "]\n";
						}
						sockets += " socket:[" + std::to_string(inode) + "]\n";
        				}
				}
			}

			ptr += size;
			readed -= size;
		}

		FreeFileData(buf);
	}

	if( !cmdline.empty() ) {
		fprintf(file, "\ncmdline: \n\n");
		fprintf(file, "%s\n", cmdline.c_str());
	}

	if( !net.empty() ) {
		fprintf(file, "\nnetwork: \n\n");
		fprintf(file, "%s", net.c_str());
	}

	if( !env.empty() ) {
		fprintf(file, "\nenviron: \n\n");
		fprintf(file, "%s\n", env.c_str());
	}

	if( !status.empty() ) {
		fprintf(file, "\nstatus: \n\n");
		fprintf(file, "%s\n", status.c_str());
	}

	if( !maps.empty() ) {
		fprintf(file, "\nmaps: \n\n");
		fprintf(file, "%s", maps.c_str());
	}

	if( !files.empty() ) {
		fprintf(file, "\nfiles: \n\n");
		fprintf(file, "%s", files.c_str());
	}

	if( !sockets.empty() ) {
		fprintf(file, "\nsockets: \n\n");
		fprintf(file, "%s", sockets.c_str());
	}

	fclose(file);

	return path;
}

Process::Process(pid_t pid_, CPUTimes & ct_):
	pid(pid_),
	ct(ct_)
{
	lasttimes = 0;
	pageSize = sysconf(_SC_PAGESIZE);
	if( pageSize == -1 ) {
		LOG_ERROR("Cannot get pagesize by sysconf(_SC_PAGESIZE) ... error (%s)\n", errorname(errno));
		pageSize = DEFAULT_PAGE_SIZE;
	}
	pageSizeKB = pageSize / ONE_K;

//	LOG_INFO("id=%u\n", pid);
	Update();
};

static CPUTimes dummy_ct_ = {0};

Process::Process(pid_t pid_):
	pid(pid_),
	ct(dummy_ct_)
{
	valid = false;
	startTimeMs = 0;
}

Process::~Process()
{
//	LOG_INFO("id=%u\n", pid);
};

void Process::Log(void) const
{
	LOG_INFO("name: %s\n", name.c_str());
	LOG_INFO("state: %c\n", state);
	LOG_INFO("ppid: %d, pgrp: %d, session: %d, tty: %d, tpgid: %d\n", ppid, pgrp, session, tty, tpgid);
	LOG_INFO("flags: 0x%08X, min_flt: %d, cmin_flt: %d, maj_flt: %d, cmaj_flt: %d\n", flags, min_flt, cmin_flt, maj_flt, cmaj_flt);
	LOG_INFO("utime %llu, stime %llu, cutime %llu, cstime %llu\n", utime, stime, cutime, cstime);
	LOG_INFO("priority: %d, nice: %d\n", priority, nice);
	LOG_INFO("pageSize: %d, pageSizeKB: %d\n", pageSize, pageSizeKB);
	LOG_INFO("m_virt: %d, m_resident: %d\n", m_virt, m_resident);
	if( startTimeMs )
		LOG_INFO("start_time: %s\n", msec_to_str(GetRealtimeMs() - startTimeMs));
}

bool Process::Kill(void) const
{
	auto buf = std::make_unique<char[]>(MAX_CMD_LEN+1);
	if( (snprintf(buf.get(), MAX_CMD_LEN+1, "kill %d", pid)) < 0 )
		return false;
	return Exec(buf.get()) == 0 ? true:RootExec(buf.get()) == 0;
}

#ifdef MAIN_PROCESS

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
	CPUTimes ct = {0};
	GetCPUTimes(&ct);
	Process proc = Process(getpid(), ct);
	proc.Log();
	for(int i=0; i< 10000; i++)
		proc.FreeFileData(proc.GetFileData("/proc/%u/stat", getpid(), 0, 0));
	GetCPUTimes(&ct);
	proc.Update();

	proc.CreateProcessInfo();

	Process proc2 = Process(1, ct);
	proc2.Update();
	auto path = proc2.CreateProcessInfo();

	return 0;
}
#endif //MAIN_PROCESS
