
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

#include <hidapi.h>
 

const uint16_t NINTENDO = 1406; // 0x057e
const uint16_t JOYCON_L = 0x2006; // 
const uint16_t JOYCON_R = 0x2007; //
const uint16_t JOYCON_GRIP = 0x200e; // 0x200e

extern int fd;
extern int silent;
extern int debug_responses;
extern unsigned char response[128];

#define DD if(debug_responses) 


unsigned char rumble_data[8];
int cmd_count = 1;


void print_buf(unsigned char *buf, int len) {
		int i;
		printf("         "); for (i = 0; i < len; i++) printf("%2d ", (int) i);
		printf("\n");

		printf("(n=%3d): ", len); for (i = 0; i < len; i++) printf("%2x ", (int) buf[i]);
		printf("\n\n");

}

void hid_list()
{
    int isLeft = 0;
    struct hid_device_info *dev;

    dev = hid_enumerate(0x0, 0x0);

    while (dev)
    {
        //printf("HID Device product_id= %x %s %s PATH=%s\n", (int)dev->product_id , dev->manufacturer_string, dev->product_string, dev->path);
        printf("Device Found: ID: %04hx:%04hx SN:%ls N:%ls PS:%ls PATH=%s\n", dev->vendor_id, dev->product_id, dev->serial_number,
         dev->manufacturer_string, dev->product_string, dev->path);

        if (1)
            if (dev->vendor_id == NINTENDO)
                if (dev->product_id == 0x200e || dev->product_id == 0x2006 || dev->product_id == 0x2007)
                {

                    //int id = devs_num;
                    //devs_num++;
                    //devs[id].path = dev->path;

                    printf("  Connected device path=%s\n", dev->path);

                }

        dev = dev->next;
    }
}

hid_device* dev;


int open_device(char *path)
{
    printf("OPEN DEVICE %s\n",path);
    dev = hid_open_path(path);

    if(dev==NULL) {
        printf(" Cannot open device\n");
        exit(0);
    }

    wchar_t str[100];
    char cstr[256];
    hid_get_serial_number_string(dev, (wchar_t*)str, 100);

    int length = wcstombs(cstr,str,256);

    int bluetooth = 1;

    if(strcmp(cstr,"000000000001") == 0) {
        bluetooth = 0;
    }

    printf("JoyCon connected SN = %s %s\n", cstr, bluetooth?"bluetooth":"usb");
}


void hid_send_command(int cmd, unsigned char *data, int len) 
{


	uint8_t buf[128];

	buf[0] = cmd;
	buf[1] = (cmd_count++) & 0xf;
	
	int i;
	for(i=0;i<8;i++) buf[ 2 + i ] = rumble_data[i];
	for(i=0;i< len ;i++) buf[ 2 + 8 + i] = data[i];

	int result = hid_write(dev, buf, 2+8 + len);
 

	if(result<0) { perror("Error writing to device"); exit(0); }

	DD printf("---- hid_send_command ---\n");
	DD print_buf(data,len);

	while(1) {
		int res = hid_read_timeout(dev, response, 64, 800);

		if (res < 0) {
			perror("read");
			exit(1);
		} 

		if(response[0]  != 0x21) continue;
		if(response[14] != data[0]) continue;

		DD if(len > 0) printf("Response to command %x:\n", data[0]);
		DD print_buf(response,res);
		break;
	}


}

int read_from_hid(char *buf, int len) {
    return hid_read_timeout(dev, buf, len, 800);
}

main1()
{

    hid_init();
    hid_list();
}
