#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int fd = 0;
    unsigned char keyVal;
    int cnt = 0;

    fd = open("/dev/thirdDrv", O_RDWR);
    if(fd < 0)
    {
        printf("can't open file\n");
        return 0;
    }

    while(1)
    {
        read(fd, &keyVal, 1);
        printf("cnt: %d, keyVal: 0x%x\n", cnt++, keyVal);
    }
    
    return 0;
}




