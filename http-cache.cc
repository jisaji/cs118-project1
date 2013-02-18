#include "http-cache.h"
#include <string>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

bool Cache::existsInCache(string path, string host)
{
	string name = path + host;
	if(access(name.c_str(), F_OK) == 0)
	{
		//File exists
		return true;
	}
	else
		return false;
}

bool Cache::isExpired(string path, string host)
{
	time_t current = time(NULL);
	string name = path + host;
	int fd = open(name.c_str(), 0);
	union date {
		int i;
		char c[4];
	}
	read(fd, date.c, 4);
	


}