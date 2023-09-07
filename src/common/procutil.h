#ifndef __COMMON_PROCUTIL_H__
#define __COMMON_PROCUTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <sys/types.h>
#include <stdint.h>

uint64_t GetRealtimeMs(void);

int getlinksrc(const char * pathname, char * real, size_t size, char **rest);
int GetFields(char * ln, const char * sep, char *** fr, int * eb, int en);

char * GetFileData(const char * path, ssize_t *readed);
void FreeFileData(char * buf);

unsigned int GetExistingCPUs(void);
unsigned int GetActiveCPUs(unsigned int existingCPUs);

typedef struct {
	unsigned long long user;
	unsigned long long nice;
	unsigned long long system;
	unsigned long long idle;
	unsigned long long iowait;
	unsigned long long irq;
	unsigned long long softirq;
	unsigned long long steal;
	unsigned long long guest;
	unsigned long long guest_nice;

	unsigned long long btime;

	unsigned long long idleall;
	unsigned long long systemall;
	unsigned long long virtall;
	unsigned long long total;

	unsigned long long user_period;
	unsigned long long nice_period;
	unsigned long long system_period;
	unsigned long long idle_period;

	unsigned long long iowait_period;
	unsigned long long irq_period;
	unsigned long long softirq_period;
	unsigned long long steal_period;
	unsigned long long guest_period;

	unsigned long long systemall_period;
	unsigned long long idleall_period;
	unsigned long long total_period;

	unsigned int activeCPUs;
	unsigned int existingCPUs;
	unsigned int runningTasks;

	double period;

} CPUTimes;

void GetCPUTimes(CPUTimes *ct);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_PROCUTIL_H__
