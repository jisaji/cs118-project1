#include <string>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int main()
{
	time_t current = time(NULL);
	string name = "test";
	int fd = open(name.c_str(), O_CREAT | O_RDWR, 0666);
	union date {
		time_t;
		char c[4];
	}
	date.t = current;
	write(fd, date.c, 4);
	return 0;
}
