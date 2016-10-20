#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char ** argv)
{
    int fd;
    int cnt = 0;
    unsigned int keyValue, old_keyValue;

    fd = open("/dev/secondDrv", O_RDWR);
    if(fd < 0)
    {
        printf("can't open file\n");
        return 0;
    }

    while(1)
    {
        read(fd, &keyValue, sizeof(keyValue));
        if(old_keyValue != keyValue)
        {
            printf("cnt: %d, key: 0x%x\n", cnt++, keyValue);
            old_keyValue = keyValue;
        }
    }

    return 0;
}
