/*
 * Hidraw Userspace Example
 *
 * Copyright (c) 2010 Alan Ott <alan@signal11.us>
 * Copyright (c) 2010 Signal 11 Software
 *
 * The code may be used by anyone for any purpose,
 * and can serve as a starting point for developing
 * applications using hidraw.
 */

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 *
 * If you need this, please have your distro update the kernel headers.
 */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

const char *bus_str(int bus);

int debug_responses = 1;
#define DD if(debug_responses) 

void print_buf(unsigned char *buf, int len) {
		int i;
		printf("         ", len); for (i = 0; i < len; i++) printf("%2d ", (int) i);
		printf("\n");

		printf("(n=%3d): ", len); for (i = 0; i < len; i++) printf("%2x ", (int) buf[i]);
		printf("\n\n");

}

void hex_dump(unsigned char *buf, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", buf[i]);
    printf("\n");
}


unsigned char response_buf[256];




int read_response(int fd, unsigned char *buf)
{
	int i;

	int res = read(fd, buf, 164);
	if (res < 0) {
		perror("read_response: Error in read ");
		return 0;
	} 
	DD print_buf(buf,res);

	return res;
}


int hidraw_write(int fd, unsigned char *buf, int n)
{
	int res = write(fd, buf, n );

	if (res < 0) {
		printf("Error in writing report: %d\n", errno);
		perror("write");
	
	}
	return res;
}	
/*
int hidraw_exchange(int fd, int step, unsigned char *buf, int n, unsigned char *response)
{
	DD printf("---- hid_send_command ---\n");
	DD print_buf(buf, n);



	if(response == NULL) {
		return 0;
	}

	int nresponse = read_response(fd, response);
	return nresponse;
}
*/

int hidraw_exchange_usb(int fd, unsigned char *buf, int n, unsigned char *response)
{

	printf("*** exchange and wait: %x %x\n",buf[0],buf[1]);
	int res = write(fd, buf, n );

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
	nresponse = read_response(fd, response);//(fd, step, buf, n, response);

	while(1) {
		printf("Wait for command n=%d \n",res); //hex_dump(buf,20);

		if(response[0] == 0x81 && response[1] == buf[1]) break;
		nresponse = read_response(fd, response);

		if(tries>10) {
				hidraw_write(fd,buf,n);
				nresponse = read_response(fd,response);
				tries = 0;
		}
		tries ++ ;
	}

	return nresponse;
}





int bluetooth = 0;

int joycon_send_command(int fd, int command, uint8_t *data, int len, uint8_t *response)
{
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    if(!bluetooth)
    {
        buf[0x00] = 0x80;
        buf[0x01] = 0x92;
        buf[0x03] = 0x31;
    }
    
    buf[bluetooth ? 0x0 : 0x8] = command;
    if(data != NULL && len != 0)
        memcpy(buf + (bluetooth ? 0x1 : 0x9), data, len);
    
    int nread = hidraw_exchange_usb(fd, buf, len + (bluetooth ? 0x1 : 0x9), response);

	return nread;
}

int global_count;

int joycon_send_subcommand(int fd, int command, int subcommand, uint8_t *data, int len, uint8_t *response)
{
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);

	printf("\n\nSubcommand: %3x %3x len=%d", command,  subcommand,len);

    
    uint8_t rumble_base[9] = {(++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40};
    memcpy(buf, rumble_base, 9);
    
    buf[9] = subcommand;
    if(data && len != 0)
        memcpy(buf + 10, data, len);
        
    int nread = joycon_send_command(fd, command, buf, 10 + len, response);        

	return nread;
}















void spi_read(int fd, uint32_t offs, uint8_t *data, uint8_t len)
{
    unsigned char buf[0x400];
    uint8_t *spi_read_cmd = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    uint32_t* offset = (uint32_t*)(&spi_read_cmd[0]);
    uint8_t* length = (uint8_t*)(&spi_read_cmd[4]);
   
    *length = len;
    *offset = offs;
   
    int max_read_count = 10;
	int read_count = 0;
    do
    {
        //usleep(300000);
		read_count += 1;
        memcpy(buf, spi_read_cmd, 0x26);
        joycon_send_subcommand(fd, 0x1, 0x10, buf, 0x26, buf);
		printf("SPIREAD:"); hex_dump(buf,64);
    }
    while(*(uint32_t*)&buf[0xF + (bluetooth ? 0 : 10)] != *offset && read_count < max_read_count);
	if(read_count > max_read_count)
        printf("ERROR: Read error or timeout\nSkipped reading of %dBytes at address 0x%05X...\n", 
            *length, *offset);

    memcpy(data, &buf[0x14 + (bluetooth ? 0 : 10)], len);
}



int fd;


void wait_for(int pos, unsigned char value) {
	int i;	
	for(i=0;i<30;i++) printf("%2d ",i);printf("\n");

	while(1) { 
		if(response_buf[0] == 0x81) hex_dump(response_buf,32);
		if(response_buf[pos] == value) break; 
		read(fd,response_buf,64);
	 }

}



#include  <time.h>

struct timeval  tv1, tv2;

void clock_start() { gettimeofday(&tv1, NULL); }

double clock_measure() {

	gettimeofday(&tv2, NULL);
	
	double time =          (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec);
		 return time;
}


int main(int argc, char **argv)
{
	int i, res, desc_size = 0;
	unsigned char buf[4096];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;


	char fname[256];

	sprintf(fname,"/dev/hidraw%s", argv[1]);


	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */
	fd = open(fname, O_RDWR);

	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));

	/* Get Report Descriptor Size */
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		perror("HIDIOCGRDESCSIZE");
	else
		printf("Report Descriptor Size: %d\n", desc_size);

	/* Get Report Descriptor */
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0) {
		perror("HIDIOCGRDESC");
	} else {
		printf("Report Descriptor:\n");
		for (i = 0; i < rpt_desc.size; i++)
			printf("%hhx ", rpt_desc.value[i]);
		puts("\n");
	}

	/* Get Raw Name */
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWNAME");
	else
		printf("Raw Name: %s\n", buf);

	/* Get Physical Location */
	res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWPHYS");
	else
		printf("Raw Phys: %s\n", buf);

	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		printf("Raw Info:\n");
		printf("\tbustype: %d (%s)\n",
			info.bustype, bus_str(info.bustype));
		printf("\tvendor: 0x%04hx\n", info.vendor);
		printf("\tproduct: 0x%04hx\n", info.product);
	}


#if 0
	/* Set Feature */
	buf[0] = 0x9; /* Report Number */
	buf[1] = 0xff;
	buf[2] = 0xff;
	buf[3] = 0xff;
	res = ioctl(fd, HIDIOCSFEATURE(4), buf);
	if (res < 0)
		perror("HIDIOCSFEATURE");
	else
		printf("ioctl HIDIOCGFEATURE returned: %d\n", res);

	/* Get Feature */
	buf[0] = 0x9; /* Report Number */
	res = ioctl(fd, HIDIOCGFEATURE(256), buf);
	if (res < 0) {
		perror("HIDIOCGFEATURE");
	} else {
		printf("ioctl HIDIOCGFEATURE returned: %d\n", res);
		printf("Report data (not containing the report number):\n\t");
		for (i = 0; i < res; i++)
			printf("%hhx ", buf[i]);
		puts("\n");
	}

	/* Send a Report to the Device */
	buf[0] = 0x1; /* Report Number */
	buf[1] = 0x77;
	res = write(fd, buf, 2);
	if (res < 0) {
		printf("Error: %d\n", errno);
		perror("write");
	} else {
		printf("write() wrote %d bytes\n", res);
	}

	#endif

	//char report1[] = { 0xA1,0xA2, 0xA3, 0xA4, 0x19, 0x01, 0x03, 0x07, 0x00, 0xA5, 0x02, 0x01, 0x7E, 0x00, 0x00, 0x00  };
	char command_0[] = { 0x80, 0x1 };

again:
	printf("\n---- INIT ----\n");
	hidraw_exchange_usb(fd, command_0, sizeof(command_0), response_buf);

	//char report1[] = { 0xA1,0xA2, 0xA3, 0xA4, 0x19, 0x01, 0x03, 0x07, 0x00, 0xA5, 0x02, 0x01, 0x7E, 0x0	hidraw_exchange(fd, 1, command_read_status, sizeof(command_read_status), buf);


	printf("joycon_init: Found, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
			response_buf[9], response_buf[8], response_buf[7], response_buf[6], response_buf[5], response_buf[4]);

	usleep(20000);

	char report2[] = { 0x80, 0x2 };
	hidraw_exchange_usb(fd,  report2, sizeof(report2), response_buf);

	/* while(buf[0] != 0x81) {
		res = read(fd, buf, 64);
		printf("Retry n=%d \n",res); hex_dump(buf,20);
		tries++;
		if(tries > 2) { hidraw_exchange_usb(fd, 2, report2, sizeof(report2), buf);  tries = 0; } 
	}*/
	
	
	usleep(20000);

	char report3[] = { 0x80, 0x3 };
	hidraw_exchange_usb(fd,  report3, sizeof(report3), response_buf);

	char report4[] = { 0x80, 0x2 };
	hidraw_exchange_usb(fd,  report4, sizeof(report4), response_buf);

	char command_enable_hid[] = { 0x80, 0x4 };
	hidraw_exchange_usb(fd,  command_enable_hid, sizeof(command_enable_hid), NULL);

	usleep(25000);

	uint8_t sn_buffer[128];

    spi_read(fd, 0x6002, sn_buffer, 0xE);
    
    printf("joycon_init: Successfully initialized %s with S/N: %c%c%c%c%c%c%c%c%c%c%c%c%c%c!\n\n\n", 
        "name", sn_buffer[0], sn_buffer[1], sn_buffer[2], sn_buffer[3], 
        sn_buffer[4], sn_buffer[5], sn_buffer[6], sn_buffer[7], sn_buffer[8], 
        sn_buffer[9], sn_buffer[10], sn_buffer[11], sn_buffer[12], 
    
        sn_buffer[13]);

	usleep(10000);

	global_count = 10;

    buf[0] = 0x01; // Lights
    joycon_send_subcommand(fd, 0x1, 0x30, buf, 1, response_buf);

	//   buf[0] = 0; // Factory mode
    //joycon_send_subcommand(fd, 0x1, 0x8, buf, 1, response_buf);
	//wait_for(0,0x81);

if(1) {	
	{ char cmd_home_light[30] = { 0x21,0x12, /*minicycle */0xf0,0x97, 0x97     }; 
	joycon_send_subcommand(fd, 0x1, 0x38, cmd_home_light, 5, response_buf );  }
	//usleep(20000);
	wait_for(0,0x81);
}

    buf[0] = 0x01; // 
    //joycon_send_subcommand(fd, 0x1, 0x48, buf, 1, response_buf);
	//wait_for(0,0x81);



    // Enable IMU data
    buf[0] = 0x01; // 
    joycon_send_subcommand(fd, 0x1, 0x40, buf, 1, response_buf);
	//wait_for(0,0x81);

    //buf[0] = 0x030; // 
   // joycon_send_subcommand(fd, 0x1, 0x3, buf, 1, response_buf);
	//wait_for(0,0x81);



    //joycon_send_subcommand(fd, 0x1, 0x20, buf, 0, response_buf);
	//wait_for(0,0x81);
   

	// Get information for joy cons
    buf[0] = 0x01; // 
    joycon_send_subcommand(fd, 0x1, 0x2, buf, 0, response_buf);
	//wait_for(0,0x81);

    joycon_send_subcommand(fd, 0x1, 0x50, buf, 0, response_buf);
	//wait_for(0,0x81);

	printf("GET DATA:"); hex_dump(response_buf,64);

	int bat =  ((int)response_buf[25]) + (((int)response_buf[26]) <<8);
	printf("Battery level = %.2f V\n",(double)bat*2.5/1000);


   	usleep(10000);


	buf[0] = 0x30;
    joycon_send_subcommand(fd, 0x1, 0x3, buf, 1, NULL);
	//wait_for(0,0x81);


	int n;
	//n = joycon_send_command(fd, 0x1f, NULL ,0 , buf);

	/* Get a report from the device */
	int cnt = 0;


	clock_start();

	int duration_n = 0;
	float duration = 0;

	for(int i=0;i<10;i++) res = read(fd, buf, 64);



//#define WEIRD_VIBRATION_TEST 0

#ifdef WEIRD_VIBRATION_TEST
while(1)
    for(int l = 0x10; l < 0x20; l++)
    {
        for(int i = 0; i < 8; i++)
        {
            for(int k = 0; k < 256; k++)
            {
                memset(buf, 0, 0x400);
                for(int j = 0; j <= 8; j++)
                {
                    buf[1+i] = 0x1;//(i + j) & 0xFF;
                }
                
                // Set frequency to increase
                buf[1+0] = k;
                
              
                joycon_send_command(fd, 0x10, (uint8_t*)buf, 0x9,response_buf);
                printf("Sent %x %x %u\n", i & 0xFF, l, k);
            }
        }
    }
#endif







	
 
	while(1) {
	
	    //joycon_send_subcommand(fd, 0x2, 0x2, buf, 0, NULL);
		int n ;
		cnt++;
		
b2:
	//	n = joycon_send_command(fd, 0x1f, buf ,0 , NULL);
		cnt = 0;
read_again:
		res = read(fd, buf, 64);
		//if (res < 0) {
			//perror("read");
		//} 

		print_buf(buf,res);

		double t = clock_measure()*1000;
		clock_start();

		duration += t;
		duration_n ++;
		if(duration_n > 40) {
			printf("FPS=%f\n", 1000 / (duration/duration_n) );
			duration = 0;
			duration_n = 0;
		//	n = joycon_send_command(fd, 0x1f, buf ,0 , NULL);
			//exit(1);
		}

//		printf("t=%f\n",clock_measure());
continue;

		printf("main: diff=%10.2f  (n=%3d):\t", t, res);
		
		if(buf[0]==0x30) for(i=0;i<10;i++)printf("%02x ",0);

		//if(buf[0]!=0x30) 
		for (i = 0; i < res; i++) printf("%02x ", (int)buf[i]);
		printf("\n");

		if(0)		if(buf[0]==0x81 && buf[1]==1) {
			printf("  going to init again");
			goto again;
		}

	
	}


	close(fd);
	return 0;
}

const char *
bus_str(int bus)
{
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}
