#include "sockets.h"

#include <stdio.h>
#include <common/log.h>
#include <common/errname.h>
#include <common/sizestr.h>
#include <common/procutil.h>
#include <limits.h>

#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <cstring>

#ifndef MAIN_SOCKETS
extern const char * LOG_FILE;
#else
const char * LOG_FILE = "";
#endif

#define LOG_SOURCE_FILE "sockets.cpp"

#if INTPTR_MAX == INT32_MAX
#define LLFMT "%llu"
#elif INTPTR_MAX == INT64_MAX
#define LLFMT "%lu"
#else
    #error "Environment not 32 or 64-bit."
#endif

Sockets::Sockets()
{
	LOG_INFO("\n");
	Update();
};

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 1024
#endif

const char * GetTcpState(unsigned long st)
{
	switch(st) {
	case TCP_ESTABLISHED: return "established";
	case TCP_SYN_SENT: return "syn sent";
	case TCP_SYN_RECV: return "syn recv";
	case TCP_FIN_WAIT1: return "fin wait1";
	case TCP_FIN_WAIT2: return "fin wait2";
	case TCP_TIME_WAIT: return "time wait";
	case TCP_CLOSE: return "close";
	case TCP_CLOSE_WAIT: return "close wait";
	case TCP_LAST_ACK: return "last ack";
	case TCP_LISTEN: return "listen";
	case TCP_CLOSING: return "closing";
	case (TCP_CLOSING+1): return "new syn recv";
	};
	return "unknown";
}

bool Sockets::UpdateTcpUdp(const char * path)
{
	auto fs = fopen(path, "r");
	if( !fs ) {
		LOG_ERROR("fopen(\"%s\") ... error (%s)\n", path, errorname(errno));
		return false;
	}

	auto buf = std::make_unique<char[]>(MAX_LINE_LENGTH+1);
	char * ptr = buf.get();
	size_t size = MAX_LINE_LENGTH+1;

	char **fp;
	int f_cnt = 12;

	const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
	char s[maxlen+1] = {0};

	while( fgets(ptr, size, fs) ) {
		if( GetFields(ptr, (f_cnt == 12) ? (const char *)0 : ":", &fp, (int *)0, 0) < f_cnt )
			continue;

		if( f_cnt == 12 ) {
			if (!fp[1]  || strcmp(fp[1],  "local_address") ||\
				!fp[2]  || strcmp(fp[2],  "rem_address") ||\
				!fp[3]  || strcmp(fp[3],  "st") ||\
				!fp[4]  || strcmp(fp[4],  "tx_queue") ||\
				!fp[5]  || strcmp(fp[5],  "rx_queue") ||\
				!fp[11] || strcmp(fp[11], "inode")) {
				LOG_WARN("WARNING: unsupported format: %s\n", ptr);
			    break;
			}
			f_cnt = 14;
			continue;
		}

		char * ep = nullptr;
		unsigned long laddr, lport, raddr, rport, state, txq, rxq, inode;
	    if( !fp[1] || !*fp[1] || (laddr = strtoul(fp[1], &ep, 16)) == ULONG_MAX || !ep || *ep )
			continue;
		ep = nullptr;
	    if( !fp[2] || !*fp[2] || (lport = strtoul(fp[2], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
	    if( !fp[3] || !*fp[3] || (raddr = strtoul(fp[3], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
	    if( !fp[4] || !*fp[4] || (rport = strtoul(fp[4], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if( !fp[5] || !*fp[5] || (state = strtoul(fp[5], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if (!fp[6] || !*fp[6] || (txq = strtoul(fp[6], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if (!fp[7] || !*fp[7] || (rxq = strtoul(fp[7], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if( !fp[13] || !*fp[13] || (inode = strtoull(fp[13], &ep, 0)) == ULONG_MAX || !ep || *ep)
			continue;

		std::string netadr(path+sizeof("/proc/net"));
		netadr += ":  ";

		if( inet_ntop(AF_INET, &laddr, s, maxlen) )
			netadr += s;

		netadr += ":" + std::to_string(lport);

		if( raddr ) {
			netadr += " -> ";
			if( inet_ntop(AF_INET, &raddr, s, maxlen) )
				netadr += s;
			netadr += ":" + std::to_string(rport);
		}

		if( path[sizeof("/proc/net")] == 't' ) {
			netadr += " (";
			netadr += GetTcpState(state);
			netadr += ")";
		}

		netadr += " (txq/rxq " + std::to_string(txq) + "/" + std::to_string(rxq) + ")";
		//LOG_INFO("%lu. %s\n", inode, netadr.c_str());

		socks[inode] = netadr;
	}

	fclose(fs);
	return true;
}

static int net6a2in6(char *as, struct in6_addr * ad)
{
	char buf[9], *ep;
	int i;
	size_t len;
/*
 * Assemble four uint32_t's from 4 X 8 hex digits into s6_addr32[].
 */
	for (i = 0, len = strlen(as);
	     (i < 4) && (len >= 8);
	     as += 8, i++, len -= 8)
	{
	    (void) strncpy(buf, as, 8);
	    buf[8] = '\0';
	    ep = (char *)NULL;
	    if ((ad->s6_addr32[i] = (uint32_t)strtoul(buf, &ep, 16)) ==  (uint32_t)UINT32_MAX || !ep || *ep)
		break;
	}
	return((*as || (i != 4) || len) ? 1 : 0);
}

bool Sockets::UpdateTcpUdp6(const char * path)
{
	auto fs = fopen(path, "r");
	if( !fs ) {
		LOG_ERROR("fopen(\"%s\") ... error (%s)\n", path, errorname(errno));
		return false;
	}

	auto buf = std::make_unique<char[]>(MAX_LINE_LENGTH+1);
	char * ptr = buf.get();
	size_t size = MAX_LINE_LENGTH+1;

	char **fp;
	int f_cnt = 12;

	const size_t maxlen = INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
	char s[maxlen+1] = {0};

	while( fgets(ptr, size, fs) ) {
		if( GetFields(ptr, (f_cnt == 12) ? (const char *)0 : ":", &fp, (int *)0, 0) < f_cnt )
			continue;

		if( f_cnt == 12 ) {
			if (!fp[1]  || strcmp(fp[1],  "local_address") ||\
				!fp[2]  || strcmp(fp[2],  "remote_address") ||\
				!fp[3]  || strcmp(fp[3],  "st") ||\
				!fp[4]  || strcmp(fp[4],  "tx_queue") ||\
				!fp[5]  || strcmp(fp[5],  "rx_queue") ||\
				!fp[11] || strcmp(fp[11], "inode")) {
				LOG_WARN("WARNING: unsupported format: %s\n", ptr);
			    break;
			}
			f_cnt = 14;
			continue;
		}

		struct in6_addr laddr = {0}, raddr = {0};
		unsigned long lport, rport, state, txq, rxq, inode;
		if (!fp[1] || !*fp[1] || net6a2in6(fp[1], &laddr))
			continue;

		char * ep = nullptr;
		if (!fp[2] || !*fp[2] || (lport = strtoul(fp[2], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		if (!fp[3] || !*fp[3] || net6a2in6(fp[3], &raddr))
			continue;
		ep = nullptr;
		if (!fp[4] || !*fp[4] || (rport = strtoul(fp[4], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		/*
		 * Get the state and queue sizes.
		 */
		ep = nullptr;
		if (!fp[5] || !*fp[5] || (state = strtoul(fp[5], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if (!fp[6] || !*fp[6] || (txq = strtoul(fp[6], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		if (!fp[7] || !*fp[7] || (rxq = strtoul(fp[7], &ep, 16)) == ULONG_MAX || !ep || *ep)
			continue;
		/*
		 * Get the inode and use it for hashing and searching.
		 */
		ep = nullptr;
		if (!fp[13] || !*fp[13] || (inode = strtoull(fp[13], &ep, 0)) == ULONG_MAX || !ep || *ep)
			continue;

		std::string netadr(path+sizeof("/proc/net"));
		netadr += ": ";

		if( inet_ntop(AF_INET6, &laddr, s, maxlen) )
			netadr += s;

		netadr += ":" + std::to_string(lport);

		uint64_t * test_raddr = (uint64_t *)&raddr;

		if( test_raddr[0] || test_raddr[1] ) {
			netadr += " -> ";
			if( inet_ntop(AF_INET6, &raddr, s, maxlen) )
				netadr += s;
			netadr += ":" + std::to_string(rport);
		}

		if( path[sizeof("/proc/net")] == 't' ) {
			netadr += " (";
			netadr += GetTcpState(state);
			netadr += ")";
		}

		netadr += " (txq/rxq " + std::to_string(txq) + "/" + std::to_string(rxq) + ")";
		//LOG_INFO("%lu. %s\n", inode, netadr.c_str());

		socks[inode] = netadr;
	}

	fclose(fs);
	return true;
}

const char * GetSocketType(uint32_t type)
{
	static const char *stype[] {
		"", // unknown
		"STREAM", // SOCK_STREAM
		"DGRAM", // SOCK_DGRAM
		"RAW", // SOCK_RAW
		"RDM", // SOCK_RDM
		"SEQPACKET", // SOCK_SEQPACKET
		"DCCP", // SOCK_DCCP
		"", // unknown 7
		"", // unknown 8
		"", // unknown 9
		"PACKET" // SOCK_PACKET
	};

	if( type < sizeof(stype)/sizeof(stype[0]) )
		return stype[type];
	return "";
}

bool Sockets::UpdateUnix(void)
{
	auto fs = fopen("/proc/net/unix", "r");
	if( !fs ) {
		LOG_ERROR("fopen(\"/proc/net/unix\") ... error (%s)\n", errorname(errno));
		return false;
	}

	auto buf = std::make_unique<char[]>(MAX_LINE_LENGTH+1);
	char * ptr = buf.get();
	size_t size = MAX_LINE_LENGTH+1;

	char **fp;
	int f_cnt = 7;
	int fl = 1;

	while( fgets(ptr, size, fs) ) {
		if( (f_cnt = GetFields(ptr, (f_cnt == 12) ? (const char *)0 : ":", &fp, (int *)0, 0)) < 8 )
			continue;
		if (fl) {
			/*
			 * Check the first line for header words.
			*/
			if (!fp[0] || strcmp(fp[0], "Num") || \
			!fp[1] || strcmp(fp[1], "RefCount") ||\
			!fp[2] || strcmp(fp[2], "Protocol") ||\
			!fp[3] || strcmp(fp[3], "Flags") ||\
			!fp[4] || strcmp(fp[4], "Type") ||\
			!fp[5] || strcmp(fp[5], "St") ||\
			!fp[6] || strcmp(fp[6], "Inode") ||\
			f_cnt < 8 || !fp[7] || strcmp(fp[7], "Path")) {
			LOG_WARN("WARNING: unsupported format: %s\n", ptr);
			break;
			}
			fl = 0;
			continue;
		}

		unsigned long inode;
		char * ep = nullptr;
		if (!fp[6] || !*fp[6] || (inode = strtoull(fp[6], &ep, 0)) == ULONG_MAX || !ep || *ep)
			continue;
		ep = nullptr;
		uint32_t type;
		if (!fp[4] || !*fp[4] || (type = (uint32_t)strtoul(fp[4], &ep, 16)) == (uint32_t)UINT32_MAX || !ep || *ep)
			type = (uint32_t)UINT_MAX;

		char * path = 0;
		size_t len = 0;
		if( f_cnt >= 8 && fp[7] && *fp[7] && (len = strlen(fp[7])) )
			path = fp[7];

		std::string s("unix: (");
		s += GetSocketType(type);
		s += ") ";
		if( path )
			s += path;
		socks[inode] = s;
	}

	fclose(fs);
	return true;
}

bool Sockets::UpdateNet(void)
{
	UpdateTcpUdp6("/proc/net/tcp6");
	UpdateTcpUdp6("/proc/net/udp6");
	return UpdateTcpUdp("/proc/net/tcp") && UpdateTcpUdp("/proc/net/udp");
}

bool Sockets::Update(void)
{
	LOG_INFO("\n");
	socks.clear();
	UpdateUnix();
	return UpdateNet();
}

Sockets::~Sockets()
{
	LOG_INFO("\n");
};


void Sockets::Log(void)
{
	for( auto & [inode, info] : socks )
		LOG_INFO(LLFMT ". %s\n", inode, info.c_str());
}

#ifdef MAIN_SOCKETS

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
	Sockets socks;
	socks.Update();
	socks.Log();
	return 0;
}
#endif //MAIN_SOCKETS
