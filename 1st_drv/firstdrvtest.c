#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


/*
*  usage:    ./firstdrvtest <on|off>
*
*/
int main(int argc, char ** argv)
{
    unsigned char val = 0;
    int fd = 0;

    fd = open("/dev/firstDrv", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}

    if(argc != 2)
    {
        printf("usage: \n");
        printf("%s <on|off>\n", argv[0]);

        return 0;
    }

    if(strcmp(argv[1], "on") == 0)
    {
        val = 1;
    }
    else if(strcmp(argv[1], "off") == 0)
    {
        val = 0;
    }

    write(fd, &val, 1);
    
    return 0;
}


