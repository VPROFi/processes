#ifndef __SOCKETS_H__
#define __SOCKETS_H__

#include <stdint.h>
#include <string>
#include <map>
#include <memory>

struct Sockets {

	std::map<uint64_t, std::string> socks;

	typedef std::map<uint64_t, std::string>::const_iterator const_iterator;
	const_iterator begin() const { return socks.begin(); };
	const_iterator end() const { return socks.end(); };
	const_iterator find(uint64_t inode) const { return socks.find(inode); };
	int size(void) const { return socks.size(); };

	const std::string & Get(uint64_t inode) const;


	bool UpdateNet(void);
	bool UpdateUnix(void);
	bool Update(void);
	void Log(void);
	Sockets();
	~Sockets();
private:
	bool UpdateTcpUdp(const char * path);
	bool UpdateTcpUdp6(const char * path);
};

#endif // __SOCKETS_H__