#include "http-cache.h"
#include <string>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

using namespace std;

union date_t 
{
	time_t t;
	char c[8];
};

bool Cache::existsInCache(string path, string host)
{
	string name = path + " " + host;
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
	string name = path + " " + host;
	int fd = open(name.c_str(), O_RDONLY);
	date_t date;
	read(fd, date.c, 8);
	close(fd);
	if(current < date.t)
	{
		//valid time
		return false;
	}
	return true;
}

void Cache::replaceExpireTime(string path, string host, string time)
{
	tm newtime;
	strptime(time.substr(0, time.length() - 4).c_str(), "%a, %d %b %Y %H:%M:%S", &newtime);
	string name = path + " " + host;
	int fd = open(name.c_str(), O_WRONLY);
	date_t date.t = mktime(&newtime);
	write(fd, date.c, 8);
}

