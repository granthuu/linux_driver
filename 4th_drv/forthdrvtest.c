#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>


int main(int argc, char **argv)
{
    int fd = 0;
    unsigned char keyVal;
    int cnt = 0;
    int ret;
	struct pollfd fds[1];

    fd = open("/dev/forthDrv", O_RDWR);
    if(fd < 0)
    {
        printf("can't open file\n");
        return 0;
    }
    
	fds[0].fd     = fd;
	fds[0].events = POLLIN;

    while(1)
    {
		ret = poll(fds, 1, 5000);
		if (ret == 0)
		{
			printf("time out\n");
		}
		else
		{
            read(fd, &keyVal, 1);
            printf("cnt: %d, keyVal: 0x%x\n", cnt++, keyVal);
		}
    }
    return 0;
}




