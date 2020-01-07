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

int silent = 0;
int debug_responses = 0;
int report_type = 8;

#define DD if(debug_responses) 


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

			if(fd < 0) {

				fprintf(stderr, "dev: %s  permission denied\n", fname);
				continue;
			}

			/* Get Raw Name */
			int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
			if (res < 0)
				perror("HIDIOCGRAWNAME");

			struct hidraw_devinfo info;
			res = ioctl(fd, HIDIOCGRAWINFO, &info);

			fprintf(stderr, "dev: %s  id %04hx:%04hx  name %s\n", fname, (unsigned int)info.vendor, (unsigned int)info.product, buf);
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
		printf("         "); for (i = 0; i < len; i++) printf("%2d ", (int) i);
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
	for(i=0;i< len ;i++) buf[ 2 + 8 + i] = data[i];

	int result = write(fd, buf, 2+8 + len);
	if(result<0) { perror("Error writing to device"); exit(0); }

	DD printf("---- hid_send_command ---\n");
	DD print_buf(data,len);

	while(1) {
		int res = read(fd, response, 64);
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


void hid_send_command_1(int cmd, unsigned char cmd_byte0) {
	char buf[2];	
	buf[0] = cmd_byte0;
	hid_send_command(cmd,buf,1);
}

void hid_send_command_2(int cmd, unsigned char cmd_byte0, unsigned char cmd_byte1) {
	char buf[2];	
	buf[0] = cmd_byte0;
	buf[1] = cmd_byte1;
	hid_send_command(cmd,buf,2);
}


unsigned char * read_spi(int addr, int len)
{
	char cmd1[10] = { 0x10,0x05,0x20,0,0,len};
	int *pnt = (int*) & cmd1[1];
	*pnt = addr;
//print_buf(cmd1,6);
	
	hid_send_command(0x1, cmd1, 6 );
	return &response[20];
}


unsigned char * write_spi(int addr, int len, char *data)
{
	char cmd1[100] = { 0x11,0x05,0x20,0,0,len};
	int *pnt = (int*) & cmd1[1];
	*pnt = addr;
	for(int i=0;i<len;i++) cmd1[6+i] = data[i];
	print_buf(cmd1,6+len);

	hid_send_command(0x1, cmd1, 6 + len );
	return &response[20];
}



void spi_dump(int addr, int len)
{

	debug_responses = 0;

	printf("        ");

	for(int i=0;i<16;i++) {
		printf("%2x ",i);
	}

	printf("\n");

	
	while(len > 0) {
		len -= 16;

		unsigned char *data = read_spi(addr,16);

		printf("%06x  ",addr);
		for(int i=0;i<16;i++) printf("%02x ", data[i]);


		printf("    ");

		for(int i=0;i<16;i++) {
			unsigned char c = data[i];
			if(c<32) c = ' ';
			if(c>128) c = ' ';
			printf("%c",c);
		}

		
		printf("\n");
		addr += 16;
	}

}


int16_t read_int(unsigned char *p) {
	int16_t num;
	unsigned char *p2 = (unsigned char *)&num;
	p2[0] = p[0];
	p2[1] = p[1];
	return num;
}


// rate= 66.7 (n= 49):      -48     -6   4122      A    -12      8     -2    |      -50     -4   4122      A    -12      7     -1     |      -60     -4   4121      A    -11      7     -2   

struct stick_calib {
	int16_t center_x;
	int16_t center_y;
	int16_t x_min;
	int16_t x_max;
	int16_t y_min;
	int16_t y_max;
	int data[6];
};

struct stick_calib calib_left, calib_right;


void stick_calib_init(struct stick_calib *s, int isleft, unsigned char *stick_cal) {
		uint16_t data[6];

  int d0 = ((stick_cal[7] << 8) & 0xf00) | stick_cal[6]; // X Min below center
  int d1  = ((stick_cal[8] << 4) | (stick_cal[7] >> 4));  // Y Min below center
  int d2  = ((stick_cal[1] << 8) & 0xf00) | stick_cal[0]; // X Max above center
  int d3 = ((stick_cal[2] << 4) | (stick_cal[1] >> 4));  // Y Max above center
  int d4 = ((stick_cal[4] << 8) & 0xf00 | stick_cal[3]); // X Center
  int d5 = ((stick_cal[5] << 4) | (stick_cal[4] >> 4));  // Y Center

  printf("D=%d %d %d %d %d %d\n",d0,d1,d2,d3,d4,d5);

  if(isleft) {
	  s->center_x = d4;
	  s->center_y = d5;
  } else {
	  s->center_x = d2;
	  s->center_y = d3;
  }

		s->x_min = (stick_cal[7] << 8) & 0xF00 | stick_cal[6];
        s->y_min = (stick_cal[8] << 4) | (stick_cal[7] >> 4) ;
	 	
		s->x_max =  (stick_cal[1] << 8) & 0xF00 | stick_cal[0];
		s->y_max =  (stick_cal[2] << 4) | (stick_cal[1] >> 4);

		printf("center: %d %d  min=%d %d max=%d %d %x %x\n",
			(int)s->center_x,(int)s->center_y,
			(int)s->x_min,(int)s->y_min,
			(int)s->x_max,(int)s->y_max,
			(int)s->center_x,(int)s->center_y
		);
}

void read_calibration()
{
	int addr = 0x6000;

	int pos = 0;
#if 0	
	unsigned char calib_data[256];

	while(pos < 256) {
		unsigned char * data;
		data = read_spi(addr,16);
		memcpy(calib_data + pos, data, 16);
		pos += 16;
		addr += 16;
	}

	for(int i=0;i<128;i++)  printf("%02x ", calib_data[i]);	
	printf("\n");
#endif
	unsigned char *data = read_spi(0x603d,16);
	stick_calib_init(&calib_left, 1, (unsigned char *)&data[0]);

	data = read_spi(0x6046,16);
	stick_calib_init(&calib_right, 0, (unsigned char *)&data[0]);
}


int open_device(char *fname)
{
	char buf[256];
	int res, desc_size, i;

	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;

	printf("\nOpening device: %s\n",fname);

	fd = open(fname, O_RDWR);

	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));
#ifdef PRINT_DEVICE_DESCRIPTOR
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
#endif
	/* Get Raw Name */
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWNAME");
	else
		printf("    Name: %s\n", buf);

	/* Get Physical Location */
	res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWPHYS");
	else
		printf("    Phys: %s\n", buf);

	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		printf("    Info: ");
		printf("bustype: %d (%s)",
			info.bustype, bus_str(info.bustype));
		printf(" vendor: 0x%04hx", info.vendor);
		printf(" product: 0x%04hx\n", info.product);
	}

	fflush(stdout);

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

extern void uinput_move(int dx, int dy);
extern void uinput_setup();
extern void uinput_update(int g0,int g1,int g2, int a0, int a1, int a2);
extern void uinput_button_press(int num, int state);
extern void uinput_stick( int stick_horizontal, int stick_vertical);

int joycon_type; // 1 Left 2 Right

int main(int argc, char **argv)
{
	int i, res, desc_size = 0;
	unsigned char buf[64];

	float scale_factor = 45;
	if(getenv("SCALE")) scale_factor = atof(getenv("SCALE"));

	
	if(argc>2) {
		report_type = atoi(argv[2]);
		printf("report_type=%d\n",report_type);
	}



	if(argc>3) {
		silent = atoi(argv[3]);
	} else { if(report_type==8) silent = 1; }


	if(argc==1) {
		printf("Device list:\n");
		hid_list();
		exit(0);
	}

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
	hid_send_command_2(1, 0x8, 0x1); // set_factory_mode 0

	hid_send_command_2( 1, 0x40, 0x1);
	hid_send_command_2( 1, 0x3, 0x30);
	hid_send_command_1( 1, 0x20);

	char cmd_gyro[10] = { 0x41, 1,0,0,0 };

	hid_send_command(0x1, cmd_gyro, 5 );

	//while(1) { read(fd,response,64); print_buf(response,50); if(response[0] == 0x21 && response[14]==0x41) break; }
	{ 
		hid_send_command_1(1, 0x50);
		int b = 2.5 * ( (  (int)response[16] ) << 8 | ((int)response[15]) );
		float voltage = (float)b/1000;
		fprintf(stderr, "Battery Voltage=%.4f PERCENT=%2.1f\n", voltage,  100*(voltage-3.1)/(4.1-3.1) ) ;
	}

	{
		hid_send_command_1( 1, 0x02);
		uint8_t *p = &response[15];
		printf("Firmware version = %d.%d  Joycon type=%d \n", (int)p[0],(int)p[1], (int)p[2]   );
		joycon_type = p[2];
	}

	{
		uint8_t *data = read_spi(0x6000,16);
		uint8_t serial[17];

		int i,j;
		for(i=0,j=0;i<16;i++) { serial[j] = data[i]; if(data[i] !=0) j++; }
		serial[j] = 0;

		printf("Serial Number: %s\n",serial);
	}


	if(report_type & 32) { // DUMP SPI FLASH
		hid_send_command_2(1,0x3,0x3f);
		spi_dump(0x2000, 128);

		spi_dump(0x3000, 128);
		spi_dump(0x4000, 128);
		spi_dump(0x5000, 16);

		spi_dump(0x6000, 128);
		read_calibration();

		exit(1);
		

		spi_dump(0x8000,128);
		char bb[10] = {0x44,0x55,1,2,3,4,5,6,7,8};
		write_spi(0x8026,1,bb);
		print_buf(response,64);
		spi_dump(0x8000,128);
		spi_dump(0x6000, 256);
		spi_dump(0x6e00, 256);

		read_calibration();
		hid_send_command_2( 1, 0x3, 0x30);
		hid_send_command_2( 1, 0x1, 0x3);

		print_buf(response,64);
		exit(1);

	}

	//exit(1);



	//int pipe_fd = open("pipe",O_WRONLY);
	udp_init();

	int prev_time;


	clock_start();
	int time_sum_n = 0;
	double time_sum;


	uinput_setup();
	read_calibration();


	/* Get a report from the device */

	int last_packet_time = -9999;
	int packets_missed = 0;

	while(1) {


		res = read(fd, buf, 64);
		if (res < 0) {
			perror("read");
			exit(1);

		} 

		// if(buf[0] == 0x31) continue;

		int time = (int)buf[1];
		
		if( last_packet_time > -9999) {
			int diff = time - last_packet_time;
			if(diff < 0) diff += 255;
			if(diff != 3) packets_missed++;
		}

		last_packet_time = time;

		if(!silent) printf("Missed %6d ",packets_missed);

		double time_spent;
		time_spent = clock_measure();
		clock_start();
		time_sum += time_spent;
		time_sum_n ++;

		if(!silent) printf("rate=%5.1f (n=%3d): ", 1/ (time_sum/time_sum_n), res);

		if(time_sum_n > 5) { time_sum_n = 0; time_sum = 0; }

		//printf(" %03d ", time-prev_time);
		prev_time = time;


		if(report_type & 1) for (i = 0; i < res; i++) printf("%02hhx ", buf[i]);

		if(!silent)  printf("  ");


		// Parse joy con hid packet 

		short int G0,G1,G2,A0,A1,A2;

		int start = 13;
		short int *p = (short int *) (&buf[start]);

		if(report_type & 2) {
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

		int left = joycon_type == 1;

		if(report_type & 4) {

			int z=0;
			char str1[256], str2[256], str3[256];

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			A0 = A0 +29; A1 = A1 -13; A2 = A2 + 1;

			printf("G %6d %6d %6d     ", (int)G0,(int)G1,(int) G2); printf(" A %6d %6d %6d  ", (int)A0,(int)A1,(int) A2);

			sprintf(str1 , "%6d %6d %6d  %6d %6d %6d\n", (int)G0,(int)G1,(int) G2, (int)A0, (int)A1, (int)A2 );

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];

			uinput_update(G0,G1,G2, left?A0:(A0),left?A1:(-A1),left?A2:(-A2));
 			
			printf("  |   %6d %6d %6d     ", (int)G0,(int)G1,(int) G2); printf(" A %6d %6d %6d   ", (int)A0,(int)A1,(int) A2);
			sprintf(str2, "%6d %6d %6d  %6d %6d %6d\n", (int)G0,(int)G1,(int) G2, (int)A0, (int)A1, (int)A2 );

			G0 = p[z++]; G1 = p[z++]; G2 = p[z++];
			A0 = p[z++]; A1 = p[z++]; A2 = p[z++];
 		
			printf("  |   %6d %6d %6d     ", (int)G0,(int)G1,(int) G2); printf(" A %6d %6d %6d   ", (int)A0,(int)A1,(int) A2);

			sprintf(str3, "%6d %6d %6d  %6d %6d %6d\n", (int)G0,(int)G1,(int) G2, (int)A0, (int)A1, (int)A2 );


			udp_send(str3);
			udp_send(str2);
			udp_send(str1);

			//click
		}


	if(report_type & 8) {

			int z=0;
			char udp_string[3][256];
			int G0[3],G1[3],G2[3];
			int A0[3],A1[3],A2[3];

			for(int i=0;i<3;i++) {
				G0[i] = p[z++]; G1[i] = p[z++]; G2[i] = p[z++];
				A0[i] = p[z++]; A1[i] = p[z++]; A2[i] = p[z++];
				
				A0[i] = A0[i] +29; A1[i] = A1[i] -13; A2[i] = A2[i] + 1;

				//uinput_update(G0,G1,G2, left?A0[i]:(A0[i]),left?A1[i]:(-A1[i]),left?A2[i]:(-A2[i]));
				//printf("G %6d %6d %6d     ", (int)G0[i],(int)G1[i],(int) G2[i]); 
				//printf(" A %6d %6d %6d  ",   (int)A0[i],(int)A1[i],(int) A2[i]);

				sprintf(udp_string[i], "%6d %6d %6d  %6d %6d %6d\n", 
				(int)G0[i],(int)G1[i],(int) G2[i],
				(int)A0[i],(int)A1[i],(int) A2[i] );
			}

				int sumA0 = (A0[0]+A0[1]+A0[2])/3;
				int sumA1 = (A1[0]+A1[1]+A1[2])/3;
				int sumA2 = (A2[0]+A2[1]+A2[2])/3;
				

			uinput_update(G0[0],G1[0],G2[0], left?sumA0:(sumA0),left?sumA1:(-sumA1),left?sumA2:(-sumA2));

			udp_send(udp_string[2]);
			udp_send(udp_string[1]);
			udp_send(udp_string[0]);

			int pos = ( left == 1 ) ? 5 : 3;
			
			uinput_button_press(1, (buf[3]<<16) | (buf[4]<<8) | buf[5]);

			uint8_t *data = buf + (left ? 6 : 9);
			uint16_t stick_horizontal =  (uint16_t)data[0] | (((int)data[1] & 0xF) << 8);
			uint16_t stick_vertical = ((uint16_t)data[1] >> 4) | ((int)data[2] << 4);

			int sx = ( (int)stick_horizontal ) ;//- 1926;
			int sy = ( (int)stick_vertical ) ;//- 2174;
			if(!silent) printf("LEFT=%d STICK %d %d\n", left, (int)sx, (int)sy);

			if(left) {
				sx -= calib_left.center_x;
				sy -= calib_left.center_y;
			} else {
				sx -= calib_right.center_x;
				sy -= calib_right.center_y;
			}
			if(!silent) printf("LEFT=%d STICK %d %d\n", left, (int)sx, (int)sy);

		 	uinput_stick( sx,sy );
		}

		if(!silent) printf("\n");
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
