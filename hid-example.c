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

#include <stdint.h>

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
#include <errno.h>
#include <dirent.h>

//extern "C" {
extern void udp_init();
extern void udp_send(char *message);
//}

const char *bus_str(int bus);

void hid_list() 
{
       DIR * d = opendir("/dev");

		struct dirent *f; 
		
		while( (f = readdir(d)) != NULL ) {
			char buf[256];
			
			if(strncmp(f->d_name, "hidraw", 6) != 0) continue;

			char fname[256] = "/dev/";
			strcat(fname,f->d_name);

			int fd = open(fname, O_RDWR);

			/* Get Raw Name */
			int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
			if (res < 0)
				perror("HIDIOCGRAWNAME");

			struct hidraw_devinfo info;
			res = ioctl(fd, HIDIOCGRAWINFO, &info);



			printf("dev: %s  id %04hx:%04hx  name %s\n", fname, (unsigned int)info.vendor, (unsigned int)info.product, buf);

		}
}


void
print_bar(int value)
{
	const int length = 20;
	const char dots = 'F';

	char str[256];
	int i,j;

	memset(str, ' ', 2*length+1);

	// [ 0..length-1] [ length+1 .. 2*length]
	str[length] = '^';

	int ndots = value / 5;
	if(ndots<0) ndots = -ndots;

	if(ndots >  length) ndots = length;
	if(ndots < -length) ndots = -length;

	if(value > 0) {
		for(i=0; i < ndots ;i++) str[length+1+i] = dots;
	}

	if(value < 0) {
		for(i=0; i < ndots ;i++) str[length-i-1] = dots;
	}

	str[2*length + 1] = 0;
	printf("%s",str);
}


int fd;


unsigned char rumble_data[8];
int cmd_count = 1;


void print_buf(unsigned char *buf, int len) {
		int i;


		printf("         ", len); for (i = 0; i < len; i++) printf("%2d ", (int) i);
		printf("\n");

		printf("(n=%3d): ", len); for (i = 0; i < len; i++) printf("%2x ", (int) buf[i]);
		printf("\n\n");

}


unsigned char response[128];


void hid_send_command(int cmd, unsigned char *data, int len) {


	uint8_t buf[128];

	buf[0] = cmd;
	buf[1] = (cmd_count++) & 0xf;
	
	int i;
	for(i=0;i<8;i++) buf[ 2 + i ] = rumble_data[i];

	for(i=0;i< len ;i++) buf[ 2+8 + i] = data[i];


	write(fd, buf, 2+8 + len);
	printf("---- hid_send_command ---\n");
	print_buf(data,len);


	while(1) {
		int res = read(fd, response, 64);
		if (res < 0) {
			perror("read");
			exit(1);

		} 

		if(response[0] == 0x31) continue;

		
		print_buf(response,res);

		break;

		

	}






}


void hid_send_command_1(int cmd, unsigned char cmd_byte0) {
	char buf[8];	
	buf[0] = cmd_byte0;
	hid_send_command(cmd,buf,1);
}

void hid_send_command_2(int cmd, unsigned char cmd_byte0, unsigned char cmd_byte1) {
	char buf[8];	
	buf[0] = cmd_byte0;
	buf[1] = cmd_byte1;
	hid_send_command(cmd,buf,2);
}


void read_spi(int addr, int len)
{
	char cmd1[10] = { 0x10,0x05,0x20,0,0,16};
	int *pnt = (int*) & cmd1[1];
	*pnt = addr;

	hid_send_command(0x1, cmd1, 6 );

}


int open_device(char *fname)
{
	char buf[256];
	int res, desc_size, i;

	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;

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
	unsigned char buf[64];

	float scale_factor = 45;
	if(getenv("SCALE")) scale_factor = atof(getenv("SCALE"));

	hid_list();

	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */

	char fname[256];

	sprintf(fname,"/dev/hidraw%s", argv[1]);

	open_device(fname);


	//char buf9[20] = {01 ,  53, 00, 00, 00, 00, 00 ,00, 00, 00,    0x8, 00}; // erase pairing
	//write(fd,buf9, 2+8 + 2);

	//char buf9[20] = {03 ,  53, 00, 00, 00, 00, 00 ,00, 00, 00,    0x8, 00}; // erase pairing
	//write(fd,buf9, 2+8 + 2);

	//char buf9[20] = {01, 53, 00, 00, 00, 00, 00 ,00, 00, 00,    0x10 , 0x10 , 0x20 , 0,0 , 10 }; // erase pairing
	//	write(fd,buf9, 2+8 + 6);

#if 0
	char cmd_reset_pairing[10] = { 0x7 };
	hid_send_command(0x1, cmd_reset_pairing, 1 ); 
	char cmd_led1[10] = { 0x30, 0xf };
	hid_send_command(0x1, cmd_led1, 2 ); 
	char cmd_reboot_and_pair[10] = { 0x6,2 };
	hid_send_command(0x1, cmd_reboot_and_pair, 2 ); 
	exit(1);
#endif


#if 0
	hid_send_command_1(1, 0x5); // Get pairing status
	hid_send_command_2(1, 0x30, 0x81); // Lights
	
	{ char cmd_home_light[10] = { 0x38, 0x21,0x13, /*minicycle */0xf0,0xff, 0xf7     }; hid_send_command(0x1, cmd_home_light, 6 );  }


	{ 
		hid_send_command_1(1, 0x50);
		int b = 2.5 * ( (  (int)response[16] ) << 8 | ((int)response[15]) );
		printf("BAT=%x %x   n=%d\n", (int)response[15], (int)response[16],(int)b);
	}

	hid_send_command_2(1, 0x8, 0x0); // set_factory_mode 1

	read_spi(0x2000, 16);
	read_spi(0x5000, 16);

	//hid_send_command_2(1, 0x6, 0x2); // Reboot and pair
	
	exit(1);
#endif

 #if 0
	char cmd_pair_bd_addr[32] = { 0x1, 0x1,  0x64  , 0x6E , 0x69 , 0xDF, 0x94 , 0xE0    };
	//printf("PAIR"); hid_send_command(0x1, cmd_pair_bd_addr, 16 );


// key = 80938783E0CA91A51828A7BD817D92F8

char link_key_cmd[20] = { 1,2,
  0x80 , 0x93 , 0x87 , 0x83 ,
  0xE0 , 0xCA , 0x91 , 0xA5 ,
  0x18 , 0x28 , 0xA7 , 0xBD ,
  0x81 , 0x7D , 0x92 , 0xF8 
 };

 	for(i=2;i<2+16;i++) link_key_cmd[i] = link_key_cmd[i] ^ 0xaa;
	//	printf("LINK"); hid_send_command(0x1, link_key_cmd, 18 );


	//char cmd_pair_bd_addr[10] = { 0x1, 0x2,  0x64  , 0x6E , 0x69 , 0xDF, 0x94 , 0xE0    };
	//hid_send_command(0x1, cmd_pair_bd_addr, 8 );

	char cmd_pair_save[12] = { 0x1, 0x3 ,0,0,0,0,0,0,0,0  };
//	hid_send_command(0x1, cmd_pair_save, 12 );

	

	char cmd1[10] = { 0x10,0x05,0x20,0,0,16};
	int *pnt = (int*) & cmd1[1];
	*pnt = 0x2000;

	char cmd2[10] = { 0x10,0x10,0x20,0,0,10};

	hid_send_command(0x1, cmd1, 6 );
	hid_send_command(0x1, cmd1, 6 );
	hid_send_command(0x1, cmd2, 6 );
	exit(1);

#endif
hid_send_command_2(1, 0x8, 0x0); // set_factory_mode 0

hid_send_command_2( 1, 0x40, 0x1);

hid_send_command_2( 1, 0x3, 0x30);
hid_send_command_1( 1, 0x20);

	//int pipe_fd = open("pipe",O_WRONLY);
	udp_init();

	int prev_time;


	clock_start();
	int time_sum_n = 0;
	double time_sum;



	/* Get a report from the device */
	while(1) {


		res = read(fd, buf, 64);
		if (res < 0) {
			perror("read");
			exit(1);

		} 

		// if(buf[0] == 0x31) continue;

		int time = (int)buf[1];

		double time_spent;
		time_spent = clock_measure();
		clock_start();
		time_sum += time_spent;
		time_sum_n ++;




		printf("rate=%5.1f (n=%3d): ", 1/ (time_sum/time_sum_n), res);

		//printf(" %03d ", time-prev_time);
		prev_time = time;


		if(1) for (i = 0; i < res; i++)
			printf("%02hhx ", buf[i]);
		printf("  ");


		// Parse joy con hid packet 

		short int G0,G1,G2,A0,A1,A2;
		int start = 13;
		short int *p = (short int *) (&buf[start]);

		if(1) {
			int z=0;

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];


			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];


			print_bar(G0/scale_factor); printf(" ");
			print_bar(G1/scale_factor); printf(" ");
			print_bar(G2/scale_factor); printf("     RRRR    ");

			print_bar(A0/scale_factor); printf(" ");
			print_bar(A1/scale_factor); printf(" ");
			print_bar(A2/scale_factor); printf(" ");

		}


		if(0) {
			int z=0;

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			printf("%6d %6d %6d     ", (int)G0,(int)G1,(int) G2);
			printf(" A %6d %6d %6d  ", (int)A0,(int)A1,(int) A2);



			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			printf("  |   %6d %6d %6d     ", (int)G0,(int)G1,(int) G2);
			printf(" A %6d %6d %6d   ", (int)A0,(int)A1,(int) A2);

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			printf("  |   %6d %6d %6d     ", (int)G0,(int)G1,(int) G2);
			printf(" A %6d %6d %6d   ", (int)A0,(int)A1,(int) A2);

		char str[256];
			sprintf(str , "%6d %6d %6d  %6d %6d %6d\n", (int)G0,(int)G1,(int) G2, (int)A0, (int)A1, (int)A2 );
			udp_send(str);

			//write(pipe_fd,str,strlen(str));
		}

		printf("\n");
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
