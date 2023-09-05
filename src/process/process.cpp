#include "process.h"

#include <common/log.h>
#include <common/errname.h>
#include <common/sizestr.h>

#include <stdlib.h>

#include <cstring>
#include <memory>

#ifndef MAIN_PROCESS
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

#define LOG_SOURCE_FILE "process.cpp"

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

	char * buf = GetFileData("/proc/%u/stat", pid, 0, 0);
	if( !buf )
		return;

	valid = false;
	do {
		char *s = strrchr(buf, '(')+1;
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

	buf = GetFileData("/proc/%u/cmdline", pid, 0, 0);
	// Maybe not enough permissions
	if( buf ) {
		cmdline = buf;
		FreeFileData(buf);
	}

	Log();

	return;
}

std::string Process::GetTempName(void) const
{
	char path[sizeof("/tmp/processXXXXXX")];
	memmove(path, "/tmp/processXXXXXX", sizeof("/tmp/processXXXXXX"));
	int f = mkstemp(&path[0]);
	close(f);
	return std::string(path);
}

char * Process::GetProcInfo(const char * info, ssize_t *readed) const
{
	std::string path(std::move(GetTempName()));
	std::string cmd("cat ");
	cmd += "/proc/";
	cmd += std::to_string(pid) + "/";
	cmd += info;
	cmd += " >>";
	cmd += path;
	if( Exec(cmd.c_str()) ) {
		if( RootExec(cmd.c_str()) ) {
			cmd = "sudo " + cmd;
			Exec(cmd.c_str());
		}
	}

	LOG_INFO("get data from: %s\n", path.c_str());
	auto buf = ::GetFileData(path.c_str(), readed);
	LOG_INFO("unlink: %s\n", path.c_str());
	unlink(path.c_str());
	return buf;
}

std::string Process::CreateProcessInfo(void)
{
	std::string path(std::move(GetTempName()));
	FILE* file = fopen(path.c_str(), "a");
	if( !file ) {
		LOG_ERROR("fopen(\"%s\", \"r\") ... error (%s)\n", path.c_str(), errorname(errno));
		return std::string();
	}

	LOG_INFO("path: %s\n", path.c_str());

	fprintf(file, "pid: %d\n", pid);
	fprintf(file, "name: %s\n", name.c_str());

	if( cmdline.empty() ) {
		char * buf = GetProcInfo("cmdline", 0);
		if( buf ) {
			cmdline = buf;
			FreeFileData(buf);
		}
	}

	fprintf(file, "cmdline: %s\n", cmdline.c_str());
	fprintf(file, "state: %c\n", state);
	fprintf(file, "ppid: %d, pgrp: %d, session: %d, tty: %d, tpgid: %d\n", ppid, pgrp, session, tty, tpgid);
	fprintf(file, "flags: 0x%08lX, min_flt: %ld, cmin_flt: %ld, maj_flt: %ld, cmaj_flt: %ld\n", flags, min_flt, cmin_flt, maj_flt, cmaj_flt);
	fprintf(file, "utime %llu, stime %llu, cutime %llu, cstime %llu\n", utime, stime, cutime, cstime);
	fprintf(file, "priority: %d, nice: %d\n", priority, nice);
	fprintf(file, "virtual memory: %s\n", size_to_str((unsigned long long)m_virt*pageSize));
	fprintf(file, "resident memory: %s\n", size_to_str((unsigned long long)m_resident*pageSize));
	if( startTimeMs )
		fprintf(file, "start_time: %s\n", msec_to_str(GetRealtimeMs() - startTimeMs));

	ssize_t readed = 0;
	char * buf = GetProcInfo("environ", &readed);

	if( buf ) {
		auto ptr = buf;
		fprintf(file, "\nenviron: \n\n");
		while( readed > 0 ) {
			auto size = strlen(ptr)+1;
			assert( readed >= size );
			if( *ptr )
				fprintf(file, "%s\n", ptr);
			ptr += size;
			readed -= size;
		}
		FreeFileData(buf);
	}


	readed = 0;
	buf = GetProcInfo("maps", &readed);
	if( buf ) {
		auto ptr = buf;
		ssize_t num = 0;
		while( num < readed ) {
			if( buf[num] == 0x0A )
				buf[num] = 0;
			num++;
		}

		fprintf(file, "\nmaps: \n\n");
		while( readed > 0 ) {
			auto size = strlen(ptr)+1;

			assert( readed >= size );

			if( size > 49 && (ptr[20] == 'x' || ptr[28] == 'x' || ptr[24] == 'x' || ptr[36] == 'x' || (size > 73 && ptr[73] == '[') || ptr[49] == '[' ) )
				fprintf(file, "%s\n", ptr);
			ptr += size;
			readed -= size;
		}

		FreeFileData(buf);		
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

CPUTimes dummy_ct_ = {0};

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
	LOG_INFO("cmdline: %s\n", cmdline.c_str());
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

int RootExec(const char * cmd)
{
	std::string _cmd("sudo /bin/sh -c \'");
	_cmd += cmd;
	_cmd += "'";
	LOG_INFO("Try exec \"%s\"\n", _cmd.c_str());
	return system(_cmd.c_str());
}

int Exec(const char * cmd)
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
