#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>


int
main(int argc, char *argv[])
{
    if(1) if(fork()==0) {
        execlp("/bin/sh","/bin/sh", "-c", "mknod /var/tmp/joycon-agent p", (char *)NULL);
    }

    usleep(500000);

    while(1) {
        FILE *f = fopen("/var/tmp/joycon-agent", "r");

        if(!f) {
            perror("cannot open /var/tmp/joycon-agent");
            exit(1);
        }

        char *fgets(char *s, int size, FILE *stream);
        char buf[256];

        while (fgets(buf, 256, f) != NULL)
        {
            printf("READ=%s",buf);

            if(1) if(fork()==0) {
                char str[256];
                for(char *p = buf;p[0] != 0; p++) if(p[0]=='\n') p[0] = 0;

                sprintf(str,"joycon /dev/hidraw%s",buf);
                printf("exec %s..\n\n",str);
                execlp("/bin/sh","/bin/sh", "-c", str, (char *)NULL);
            }   

        }
        //exit(0);

        fclose(f);
    }

    return 0;
}

