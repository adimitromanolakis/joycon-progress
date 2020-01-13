
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
 
#define TRY(A,B) A
typedef uint8_t u8;



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


extern char *supported_device[256];

void hid_list()
{
    int isLeft = 0;
    struct hid_device_info *dev;

    dev = hid_enumerate(0x0, 0x0);
    supported_device[0] = 0;


    while (dev)
    {
        //printf("HID Device product_id= %x %s %s PATH=%s\n", (int)dev->product_id , dev->manufacturer_string, dev->product_string, dev->path);
        printf("    %04hx:%04hx SN:%ls N:%ls PS:%ls path=%s\n", dev->vendor_id, dev->product_id, dev->serial_number,
         dev->manufacturer_string, dev->product_string, dev->path);

        if (1)
            if (dev->vendor_id == NINTENDO)
                if (dev->product_id == 0x200e || dev->product_id == 0x2006 || dev->product_id == 0x2007)
                {
                    
                    if(supported_device[0] == 0) {
                        strncpy(supported_device,dev->path,strlen(dev->path)+1);
                        printf("      supported dev=%s\n",supported_device);
                    }

                    //int id = devs_num;
                    //devs_num++;
                    //devs[id].path = dev->path;
                    //printf("  Connected device path=%s\n", dev->path);
                }

        dev = dev->next;
    }
}

hid_device* dev;
int bluetooth = 1;

int hid_send_raw(unsigned char *data, int len, unsigned char *response)
{
    DD printf("Send command: %x %x\n", data[0],data[1]);
    DD print_buf(data, len);

	DD printf("*** Wait for response:\n");
	int res = hid_write(dev, data, len);

	if (res < 0) {
		printf("Error in writing report: %d\n", errno);
		perror("write");
		return res;
	}

	if(response == NULL) {
		return 0;
	}

	int nresponse;
	int tries = 0;

	while(1) {
        nresponse = hid_read_timeout(dev, response, 128, 800);

        DD printf("Response:\n");
        DD print_buf(response,nresponse);

		if(response[0] == 0x81 && response[1] == data[1]) break;

		if(tries>10) {
				res = hid_write(dev, data, len);
				tries = 0;
		}
		tries ++ ;
	}

    DD printf("Received an accepted response\n");

	return nresponse;
}

uint8_t * hid_send_command(int cmd, unsigned char *data, int len) 
{
	uint8_t buf[128];
    memset(buf,0,128);
    int start_pos = 0;
    
    if(!bluetooth) {
        buf[0x00] = 0x80;
        buf[0x01] = 0x92;
        buf[0x03] = 0x31;
        start_pos = 8;
    }


    uint8_t *buf_start = &buf[start_pos];

	buf_start[0] = cmd;
	buf_start[1] = (cmd_count++) & 0xf;
	
	int i;
	for(i=0;i<8;i++) buf_start[ 2 + i ] = rumble_data[i];
	for(i=0;i< len ;i++) buf_start[ 2 + 8 + i] = data[i];


	int result = hid_write(dev, buf, 2 + 8 + len + start_pos);
 
	if(result<0) { perror("Error writing to device"); exit(0); }

	DD printf("---- hid_send_command ---\n");
	DD print_buf(buf,2 + 8 + len + start_pos);

	while(1) {
		int res = hid_read_timeout(dev, response, 64, 800);
        DD printf("---- response:\n");
		DD print_buf(response,res);

		if (res < 0) {
			perror("read");
			exit(1);
		} 

        int response_pos = 24;
        if (bluetooth) response_pos = 14;

		if(bluetooth && response[0]  != 0x21) continue;
		if(!bluetooth && response[0]  != 0x81) continue;
		
        if(response[response_pos] != data[0]) continue;

        if (bluetooth) return &response[15];
        return &response[25];
		//DD if(response[0] != 0x30) if(len > 0) printf("Response to command %x:\n", data[0]);
		break;
	}
}

int read_from_hid(char *buf, int len) {
    int n = hid_read_timeout(dev, buf, len, 800);
    //print_buf(buf,n);
    return n;
}



int open_device(char *path)
{
    printf("Open device %s\n",path);
    dev = hid_open_path(path);

    if(dev==NULL) {
        printf(" Cannot open device\n");
        exit(0);
    }

    wchar_t str[100];
    char cstr[256];
    hid_get_serial_number_string(dev, (wchar_t*)str, 100);

    int length = wcstombs(cstr,str,256);


    if(strcmp(cstr,"000000000001") == 0) {
        bluetooth = 0;
    }

    printf("JoyCon connected SN = %s %s\n", cstr, bluetooth?"bluetooth":"usb");


    char response[128];

    if (!bluetooth) {
        u8 command_0[] = { 0x80, 0x1 };

        TRY(hid_send_raw(command_0, 2, response), "init usb");

        u8 report2[] = { 0x80, 0x2 };
        TRY(hid_send_raw(report2, 2, response), "init usb");

        u8 report3[] = { 0x80, 0x3 };
        TRY(hid_send_raw(report3, 2, response), "init usb");

        u8 report4[] = { 0x80, 0x2 };
        TRY(hid_send_raw(report4, 2, response), "init usb 2");

        u8 command_enable_hid[] = { 0x80, 0x4 };
        hid_send_raw(command_enable_hid, 2, NULL);
    }
}


main1()
{

    hid_init();
    hid_list();
}
