#include "procutil.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/stat.h>
#include <dirent.h>

#include "errname.h"
#include "log.h"

#define LOG_SOURCE_FILE "fileutil.c"

#ifdef MAIN_COMMON_PROCUTIL
const char * LOG_FILE = "";
#else
extern const char * LOG_FILE;
#endif

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%lld"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%ld"
#else
    #error "Environment not 32 or 64-bit."
#endif

void FreeFileData(char * buf)
{
	free((void *)buf);
}

char * GetFileData(const char * path, ssize_t *readed)
{
	#define buffGRW 1024
	size_t size = buffGRW;
	ssize_t tot_read = 0;

	char * buf = (char *)malloc(size);
	if( !buf ) {
		LOG_ERROR("malloc(" LLFMT ") ... error (%s)\n", size, errorname(errno));
		return 0;
	}

	do {
		ssize_t cur_read;
		int fd = open(path, O_RDONLY, 0);
		if( -1 == fd ) {
			LOG_ERROR("open(%s) ... error (%s)\n", path, errorname(errno));
			free((void *)buf);
			buf = 0;
			break;
		}

		while( (cur_read = read(fd, buf + tot_read, size - tot_read)) > 0 ) {
        		tot_read += cur_read;
		        if( tot_read < size ) break;
		        buf = (char *)realloc(buf, (size += buffGRW));
			if( !buf ) {
				LOG_ERROR("malloc(" LLFMT ") ... error (%s)\n", size, errorname(errno));
				break;
			}
		};

		close(fd);

		if( !buf )
			break;

		if( tot_read < 1 ) {
			LOG_ERROR("read(%s) ... error (%s)\n", path, errorname(errno));
			free((void *)buf);
			buf = 0;
			break;
		}

		buf[tot_read] = '\0';
		if( readed )
			*readed = tot_read;

	} while(0);
	#undef buffGRW

	return buf;
}

unsigned int GetExistingCPUs(void)
{
	DIR * cpu = opendir("/sys/devices/system/cpu");
	unsigned int existingCPUs = 0;
	if( cpu ) {
		const struct dirent* entry;
		while( (entry = readdir(cpu) ) != NULL) {

			if( entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN)
				continue;

			if( memcmp(entry->d_name, "cpu", 3) == 0 ) {
				char* endp;
				unsigned long int id = strtoul(entry->d_name + 3, &endp, 10);
				if( id == ULONG_MAX || endp == entry->d_name + 3 || *endp != '\0')
					continue;
				existingCPUs++;
			}
		};
	}
	closedir(cpu);
	return existingCPUs ? existingCPUs:1;
}

unsigned int GetActiveCPUs(unsigned int existingCPUs)
{
	unsigned int i = 0, activeCPUs = 0;
	if( existingCPUs == 0 )
		existingCPUs = GetExistingCPUs();
	for( i=0; i < existingCPUs; i++ ) {
		char path[sizeof("/sys/devices/system/cpu/cpu4294967296/online")];
		if( snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/online", i) < 0 ) {
			LOG_ERROR("snprintf(fmt=\"/sys/devices/system/cpu/cpu%d/online\") ... error (%s)\n", i, errorname(errno));
			continue;
		} else {
			struct stat sbuf;
			if( stat(path, &sbuf) < 0 )
		         activeCPUs++;
			else {
				char * buf = GetFileData(path, 0);
				if( !buf || buf[0] != '0' )
				         activeCPUs++;
				FreeFileData(buf);
			}
		}
	}
	return activeCPUs ? activeCPUs:1;
}

/* This subtraction is used by Linux / NetBSD / OpenBSD for calculation of CPU usage items. */
static inline unsigned long long saturatingSub(unsigned long long a, unsigned long long b) {
   return a > b ? a - b : 0;
}

#ifndef PROC_LINE_LENGTH
#define PROC_LINE_LENGTH 4096
#endif

void GetCPUTimes(CPUTimes *ct)
{
	char buffer[PROC_LINE_LENGTH + 1];
	FILE* file = fopen("/proc/stat", "r");
	CPUTimes prev_ct = *ct;
	if( !file ) {
		LOG_ERROR("fopen(\"/proc/stat\", \"r\") ... error (%s)\n", errorname(errno));
		return;
	}

	if( !fgets(buffer, sizeof(buffer), file) ||
		memcmp(buffer, "cpu", 3) != 0 ) {
		LOG_ERROR("can`t process \"/proc/stat\" file\n");
		fclose(file);
		return;
	}

	memset(ct, 0, sizeof(CPUTimes));

	ct->existingCPUs = GetExistingCPUs();
	ct->activeCPUs = GetActiveCPUs(ct->existingCPUs);
	ct->period = 0.0F;

	(void)sscanf(buffer, "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
			&ct->user,
			&ct->nice,
			&ct->system,
			&ct->idle,
			&ct->iowait,
			&ct->irq,
			&ct->softirq,
			&ct->steal,
			&ct->guest,
			&ct->guest_nice);

	// Guest time is already accounted in usertime
	ct->user -= ct->guest;
	ct->nice -= ct->guest_nice;

	// Fields existing on kernels >= 2.6
	// (and RHEL's patched kernel 2.4...)
	ct->idleall = ct->idle + ct->iowait;
	ct->systemall = ct->system + ct->irq + ct->softirq;
	ct->virtall = ct->guest + ct->guest_nice;
	ct->total = ct->user + ct->nice + ct->systemall + ct->idleall + ct->steal + ct->virtall;

//	if( prev_ct.user | prev_ct.system | prev_ct.idle ) {
		// Since we do a subtraction (usertime - guest) and cputime64_to_clock_t()
		// used in /proc/stat rounds down numbers, it can lead to a case where the
		// integer overflow.
		ct->user_period = saturatingSub(ct->user, prev_ct.user);
		ct->nice_period = saturatingSub(ct->nice, prev_ct.nice);
		ct->system_period = saturatingSub(ct->system, prev_ct.system);
		ct->systemall_period = saturatingSub(ct->systemall, prev_ct.systemall);
		ct->idleall_period = saturatingSub(ct->idleall, prev_ct.idleall);
		ct->idle_period = saturatingSub(ct->idle, prev_ct.idle);
		ct->iowait_period = saturatingSub(ct->iowait, prev_ct.iowait);
		ct->irq_period = saturatingSub(ct->irq, prev_ct.irq);
		ct->softirq_period = saturatingSub(ct->softirq, prev_ct.softirq);
		ct->steal_period = saturatingSub(ct->steal, prev_ct.steal);
		ct->guest_period = saturatingSub(ct->virtall, prev_ct.guest);
		ct->total_period = saturatingSub(ct->total, prev_ct.total);
		assert( ct->activeCPUs >= 1 );
		ct->period = ((double)ct->total_period) / (double)ct->activeCPUs;
//	}

	while( fgets(buffer, sizeof(buffer), file) ) {
		if( memcmp(buffer, "procs_running", 3) != 0 ) {
			ct->runningTasks = strtoul(buffer + strlen("procs_running"), NULL, 10);
			break;
		}
	}

	fclose(file);

	LOG_INFO("user             : %lld\n", ct->user             );
	LOG_INFO("nice             : %lld\n", ct->nice             );
	LOG_INFO("system           : %lld\n", ct->system           );
	LOG_INFO("idle             : %lld\n", ct->idle             );
	LOG_INFO("iowait           : %lld\n", ct->iowait           );
	LOG_INFO("irq              : %lld\n", ct->irq              );
	LOG_INFO("softirq          : %lld\n", ct->softirq          );
	LOG_INFO("steal            : %lld\n", ct->steal            );
	LOG_INFO("guest            : %lld\n", ct->guest            );
	LOG_INFO("guest_nice       : %lld\n", ct->guest_nice       );
	LOG_INFO("user_period      : %lld\n", ct->user_period      );
	LOG_INFO("nice_period      : %lld\n", ct->nice_period      );
	LOG_INFO("system_period    : %lld\n", ct->system_period    );
	LOG_INFO("systemall_period : %lld\n", ct->systemall_period );
	LOG_INFO("idleall_period   : %lld\n", ct->idleall_period   );
	LOG_INFO("idle_period      : %lld\n", ct->idle_period      );
	LOG_INFO("iowait_period    : %lld\n", ct->iowait_period    );
	LOG_INFO("irq_period       : %lld\n", ct->irq_period       );
	LOG_INFO("softirq_period   : %lld\n", ct->softirq_period   );
	LOG_INFO("steal_period     : %lld\n", ct->steal_period     );
	LOG_INFO("guest_period     : %lld\n", ct->guest_period     );
	LOG_INFO("total_period     : %lld\n", ct->total_period     );
	LOG_INFO("period           : %f\n",   ct->period           );
	LOG_INFO("runningTasks     : %d\n",   ct->runningTasks     );
	LOG_INFO("activeCPUs       : %d\n",   ct->activeCPUs     );
	LOG_INFO("existingCPUs     : %d\n",   ct->existingCPUs     );

	return;
}

#ifdef MAIN_COMMON_PROCUTIL

int main(int argc, char * argv[])
{
	CPUTimes ct = {0};
	ssize_t readed;
	char * buf = GetFileData("/proc/self/stat", &readed);
	if( buf ) {
		LOG_INFO("/proc/self/stat %s", buf);
		FreeFileData(buf);
	}

	LOG_INFO("existingCPUs: %d\n", GetExistingCPUs());
	LOG_INFO("activeCPUs: %d\n", GetActiveCPUs(0));
	GetCPUTimes(&ct);

	return 0;
}

#endif //MAIN_COMMON_PROCUTIL
