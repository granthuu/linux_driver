#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


#define NONBLOCK_ENABLE     0

/* fifthdrvtest */
int fd;

void my_signal_fun(int signum)
{
	unsigned char key_val;
	read(fd, &key_val, 1);
	printf("key_val: 0x%x\n", key_val);
}

int main(int argc, char **argv)
{
	unsigned char key_val;
	int ret;
	int Oflags;

	//signal(SIGIO, my_signal_fun);


#if NONBLOCK_ENABLE > 0
	fd = open("/dev/sixthDrv", O_RDWR | O_NONBLOCK);
#else
	fd = open("/dev/sixthDrv", O_RDWR);
#endif

	if (fd < 0)
	{
		printf("can't open!\n");
	}

	//fcntl(fd, F_SETOWN, getpid());
	//Oflags = fcntl(fd, F_GETFL); 
	//fcntl(fd, F_SETFL, Oflags | FASYNC);  

	while (1)
	{
		//sleep(1000);
		
		ret = read(fd, &key_val, 1);
		printf("key_val: 0x%x, ret = %d\n", key_val, ret);

#if  NONBLOCK_ENABLE > 0
       sleep(5);
#else	
		//sleep(5);
#endif
	}
	
	return 0;
}





