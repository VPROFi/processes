#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <common/procutil.h>

#include <assert.h>

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

extern int RootExec(const char * cmd);
extern int Exec(const char * cmd);

struct Process {

	pid_t pid;
	CPUTimes & ct;

	unsigned long long lasttimes;

	int pageSize;
	int pageSizeKB;


	std::string name;

	float percent_cpu;

	/* Total program size (in kilobytes) */
	long m_virt;

	/* Resident set size (in kilobytes) */
	long m_resident;

	long m_share;
	long m_trs;
	long m_drs;



	////////////////////////////////////////////////////////////////////


	long m_pss;
	long m_swap;
	long m_psswp;
	long m_lrs;

	pid_t ppid;	// stat,status     pid of parent process

	pid_t tid;      // (special)       task id, the POSIX thread ID (see also: tgid)

	// //////////////////////////////////////////////////////////////////
	// Namespace Particulars ////////////////////////////////////////////

	enum namespace_type {
	    PROCPS_NS_CGROUP,
	    PROCPS_NS_IPC,
	    PROCPS_NS_MNT,
	    PROCPS_NS_NET,
	    PROCPS_NS_PID,
	    PROCPS_NS_TIME,
	    PROCPS_NS_USER,
	    PROCPS_NS_UTS,
	    PROCPS_NS_COUNT  // total namespaces (fencepost)
	};

	struct procps_ns {
		unsigned long ns[PROCPS_NS_COUNT];
	};

	unsigned long long
	    utime,          // stat            user-mode CPU time accumulated by process
	    stime,          // stat            kernel-mode CPU time accumulated by process
	    cutime,         // stat            cumulative utime of process and reaped children
	    cstime,         // stat            cumulative stime of process and reaped children
	    start_time,     // stat            start time of process -- seconds since system boot
	    blkio_tics,     // stat            time spent waiting for block IO
	    gtime,          // stat            guest time of the task in jiffies
	    cgtime;         // stat            guest time of the task children in jiffies


	char state;	// stat,status     single-char code for process state (S=sleeping)

	int                 // next 3 fields are NOT filled in by readproc
	    pcpu,           // stat (special)  elapsed tics for %CPU usage calculation
	    maj_delta,      // stat (special)  major page faults since last update
	    min_delta;      // stat (special)  minor page faults since last update

	char
	    // Linux 2.1.7x and up have 64 signals. Allow 64, plus '\0' and padding.
	    signal[18],     // status          mask of pending signals
	    blocked[18],    // status          mask of blocked signals
	    sigignore[18],  // status          mask of ignored signals
	    sigcatch[18],   // status          mask of caught  signals
	    _sigpnd[18];    // status          mask of PER TASK pending signals
	
	unsigned long
	    start_code,     // stat            address of beginning of code segment
	    end_code,       // stat            address of end of code segment
	    start_stack,    // stat            address of the bottom of stack for the process
	    kstk_esp,       // stat            kernel stack pointer
	    kstk_eip,       // stat            kernel instruction pointer
	    wchan,          // stat (special)  address of kernel wait channel proc is sleeping in
	    rss,            // stat            identical to 'resident'
	    alarm;          // stat            ?

	int
	    priority,       // stat            kernel scheduling priority
	    nice;           // stat            standard unix nice level of process
	unsigned long
	// the next 7 members come from /proc/#/statm
	    size,           // statm           total virtual memory (as # pages)
	    resident,       // statm           resident non-swapped memory (as # pages)
	    share,          // statm           shared (mmap'd) memory (as # pages)
	    trs,            // statm           text (exe) resident set (as # pages)
	    lrs,            // statm           library resident set (always 0 w/ 2.6)
	    drs,            // statm           data+stack resident set (as # pages)
	    dt;             // statm           dirty pages (always 0 w/ 2.6)

	unsigned long
	    vm_size,        // status          equals 'size' (as kb)
	    vm_lock,        // status          locked pages (as kb)
	    vm_rss,         // status          equals 'rss' and/or 'resident' (as kb)
	    vm_rss_anon,    // status          the 'anonymous' portion of vm_rss (as kb)
	    vm_rss_file,    // status          the 'file-backed' portion of vm_rss (as kb)
	    vm_rss_shared,  // status          the 'shared' portion of vm_rss (as kb)
	    vm_data,        // status          data only size (as kb)
	    vm_stack,       // status          stack only size (as kb)
	    vm_swap,        // status          based on linux-2.6.34 "swap ents" (as kb)
	    vm_exe,         // status          equals 'trs' (as kb)
	    vm_lib,         // status          total, not just used, library pages (as kb)
	    vsize,          // stat            number of pages of virtual memory ...
	    rss_rlim,       // stat            resident set size limit?
	    flags,          // stat            kernel flags for the process
	    min_flt,        // stat            number of minor page faults since process start
	    maj_flt,        // stat            number of major page faults since process start
	    cmin_flt,       // stat            cumulative min_flt of process and child processes
	    cmaj_flt,       // stat            cumulative maj_flt of process and child processes
	    rchar,          // io              characters read
	    wchar,          // io              characters written
	    syscr,          // io              number of read I/O operations
	    syscw,          // io              number of write I/O operations
	    read_bytes,     // io              number of bytes fetched from the storage layer
	    write_bytes,    // io              number of bytes sent to the storage layer
	    cancelled_write_bytes, // io       number of bytes truncating pagecache
	    smap_Rss,              // smaps_rollup  mapping currently resident in RAM
	    smap_Pss,              //    "     Rss divided by total processes sharing it
	    smap_Pss_Anon,         //    "     proportional share of 'anonymous' memory
	    smap_Pss_File,         //    "     proportional share of 'file' memory
	    smap_Pss_Shmem,        //    "     proportional share of 'shmem' memory
	    smap_Shared_Clean,     //    "     unmodified shared memory
	    smap_Shared_Dirty,     //    "     altered shared memory
	    smap_Private_Clean,    //    "     unmodified private memory
	    smap_Private_Dirty,    //    "     altered private memory
	    smap_Referenced,       //    "     memory marked as referenced/accessed
	    smap_Anonymous,        //    "     memory not belonging to any file
	    smap_LazyFree,         //    "     memory marked by madvise(MADV_FREE)
	    smap_AnonHugePages,    //    "     memory backed by transparent huge pages
	    smap_ShmemPmdMapped,   //    "     shmem/tmpfs memory backed by huge pages
	    smap_FilePmdMapped,    //    "     file memory backed by huge pages
	    smap_Shared_Hugetlb,   //    "     hugetlbfs backed memory *not* counted in Rss/Pss
	    smap_Private_Hugetlb,  //    "     hugetlbfs backed memory *not* counted in Rss/Pss
	    smap_Swap,             //    "     swapped would-be-anonymous memory (includes swapped out shmem)
	    smap_SwapPss,          //    "     the proportional share of 'Swap' (excludes swapped out shmem)
	    smap_Locked;           //    "     memory amount locked to RAM

	char
	    *environ,       // (special)       environment as string (/proc/#/environ)
	    *cmdline,       // (special)       command line as string (/proc/#/cmdline)
	    *cgroup,        // (special)       cgroup as string (/proc/#/cgroup)
	    *cgname,        // (special)       name portion of above (if possible)
	    *supgid,        // status          supplementary gids as comma delimited str
	    *supgrp,        // supp grp names as comma delimited str, derived from supgid
	   **environ_v,     // (special)       environment string vectors (/proc/#/environ)
	   **cmdline_v,     // (special)       command line string vectors (/proc/#/cmdline)
	   **cgroup_v;      // (special)       cgroup string vectors (/proc/#/cgroup)

	char
	    *euser,         // stat(),status   effective user name
	    *ruser,         // status          real user name
	    *suser,         // status          saved user name
	    *fuser,         // status          filesystem user name
	    *rgroup,        // status          real group name
	    *egroup,        // status          effective group name
	    *sgroup,        // status          saved group name
	    *fgroup,        // status          filesystem group name
	    *cmd;           // stat,status     basename of executable file in call to exec(2)

	int
	    rtprio,         // stat            real-time priority
	    sched,          // stat            scheduling class
	    pgrp,           // stat            process group id
	    session,        // stat            session id
	    nlwp,           // stat,status     number of threads, or 0 if no clue
	    tgid,           // (special)       thread group ID, the POSIX PID (see also: tid)
	    tty;            // stat            full device number of controlling terminal
	    /* FIXME: int uids & gids should be uid_t or gid_t from pwd.h */
	    uid_t euid; gid_t egid; // stat(),status effective
	    uid_t ruid; gid_t rgid; // status        real
	    uid_t suid; gid_t sgid; // status        saved
	    uid_t fuid; gid_t fgid; // status        fs (used for file access only)
	int
	    tpgid,          // stat            terminal process group id
	    exit_signal,    // stat            might not be SIGCHLD
	    processor;      // stat            current (or most recent?) CPU
	int
	    oom_score,      // oom_score       (badness for OOM killer)
	    oom_adj;        // oom_adj         (adjustment to OOM score)
	struct procps_ns ns; // (ns subdir)    inode number of namespaces
	char
	    *sd_mach,       // n/a             systemd vm/container name
	    *sd_ouid,       // n/a             systemd session owner uid
	    *sd_seat,       // n/a             systemd login session seat
	    *sd_sess,       // n/a             systemd login session id
	    *sd_slice,      // n/a             systemd slice unit
	    *sd_unit,       // n/a             systemd system unit id
	    *sd_uunit;      // n/a             systemd user unit id
	char
	    *lxcname,       // n/a             lxc container name
	    *exe;           // exe             executable path + name
	int
	    luid,           // loginuid        user id at login
	    autogrp_id,     // autogroup       autogroup number (id)
	    autogrp_nice;   // autogroup       autogroup nice value

	bool valid;

	void Log(void) const;
	void Update(void);
	bool Kill(void) const;
	void FreeFileData(char * buf) const;
	char * GetFileData(const char * fmt, pid_t _ppid, pid_t _tpid, ssize_t *readed) const;
	explicit Process(pid_t pid, CPUTimes & ct);
	explicit Process(pid_t pid);
	~Process();
protected:
};

#endif // __PROCESS_H__