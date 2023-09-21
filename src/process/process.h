#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <common/procutil.h>

#include <assert.h>
#include <vector>

#ifndef MINIMUM
#define MINIMUM(a, b)                  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAXIMUM
#define MAXIMUM(a, b)                  ((a) > (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, low, high)            (assert((low) <= (high)), ((x) > (high)) ? (high) : MAXIMUM(x, low))
#endif

#ifndef DEFAULT_PAGE_SIZE
#define DEFAULT_PAGE_SIZE 4096
#endif

#ifndef ONE_K
#define ONE_K 1024UL
#endif

#ifndef MAX_CMD_LEN
#define MAX_CMD_LEN 1024
#endif

#ifndef PF_KTHREAD
#define PF_KTHREAD 0x00200000
#endif

extern int RootExec(const char * cmd, int flags = 0);
extern int Exec(const char * cmd, int flags = 0);

struct Process {

	pid_t pid;
	CPUTimes & ct;

	bool valid;

	std::string name;

	std::string user;
	std::string group;

	unsigned long long lasttimes;
	unsigned long long startTimeMs;

	int pageSize;
	int pageSizeKB;

	float percent_cpu;

	/* Total program size (in kilobytes) */
	long m_virt;

	/* Resident set size (in kilobytes) */
	long m_resident;

	// stat,status     single-char code for process state (S=sleeping)
	char state;

	// stat,status     pid of parent process
	pid_t ppid;
	// (special)       task id, the POSIX thread ID (see also: tgid)
	pid_t tid;

	// stat            terminal process group id
	int tpgid;

	// stat            might not be SIGCHLD
	int exit_signal;

	// stat            current (or most recent?) CPU
	int processor;

	// stat            number of pages of virtual memory ...
	unsigned long vsize;

	// stat            identical to 'resident'
	unsigned long rss;

	// stat            resident set size limit?
	unsigned long rss_rlim;

	unsigned long
	    start_code,     // stat            address of beginning of code segment
	    end_code,       // stat            address of end of code segment
	    start_stack,    // stat            address of the bottom of stack for the process
	    kstk_esp,       // stat            kernel stack pointer
	    kstk_eip,       // stat            kernel instruction pointer
	    wchan,          // stat (special)  address of kernel wait channel proc is sleeping in
	    alarm;          // stat            ?*/

	uint64_t bytesReceived;
	uint64_t packetsReceived;
	uint64_t bytesTransmitted;
	uint64_t packetsTransmitted;

	long m_share;
	long m_trs;
	long m_drs;

	unsigned long
	    flags,          // stat            kernel flags for the process
	    min_flt,        // stat            number of minor page faults since process start
	    maj_flt,        // stat            number of major page faults since process start
	    cmin_flt,       // stat            cumulative min_flt of process and child processes
	    cmaj_flt;       // stat            cumulative maj_flt of process and child processes

	unsigned long long
	    utime,          // stat            user-mode CPU time accumulated by process
	    stime,          // stat            kernel-mode CPU time accumulated by process
	    cutime,         // stat            cumulative utime of process and reaped children
	    cstime,         // stat            cumulative stime of process and reaped children
	    start_time,     // stat            start time of process -- seconds since system boot
	    blkio_tics,     // stat            time spent waiting for block IO
	    gtime,          // stat            guest time of the task in jiffies
	    cgtime;         // stat            guest time of the task children in jiffies

	int
	    priority,       // stat            kernel scheduling priority
	    nice;           // stat            standard unix nice level of process

	int
	    rtprio,         // stat            real-time priority
	    sched,          // stat            scheduling class*/
	    pgrp,           // stat            process group id
	    session,        // stat            session id
	    nlwp,           // stat,status     number of threads, or 0 if no clue
	    tgid,           // (special)       thread group ID, the POSIX PID (see also: tid)
	    tty;            // stat            full device number of controlling terminal

	void Log(void) const;
	void Update(void);
	bool Kill(void) const;

	std::string CreateProcessInfo(const char * filePath = nullptr);
	std::string GetTempName(void) const;

	char * GetFileData(const char * fmt, pid_t _ppid, pid_t _tpid, ssize_t *readed) const;
	void FreeFileData(char * buf) const;
	explicit Process(pid_t pid_, CPUTimes & ct_);
	explicit Process(pid_t pid);
	~Process();
protected:
	char * GetCommandOutput(std::string & cmd, ssize_t *readed) const;
	char * GetProcInfo(const char * info, ssize_t *readed) const;
	char * GetFilesInfo(ssize_t *readed) const;
	void CapabilitiesToString(std::string & str, uint64_t cap) const;
	std::string GetProcStatus(void) const;
	std::string GetProcInfoToString(const char * field, const char * separator) const;
};

#endif // __PROCESS_H__
