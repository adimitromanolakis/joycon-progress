#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>


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

unsigned char rumble_data[8];
int cmd_count = 1;

extern int fd;
extern int silent;
extern int debug_responses;
extern unsigned char response[128];

#define DD if(debug_responses) 



void print_buf(unsigned char *buf, int len) {
		int i;
		printf("         "); for (i = 0; i < len; i++) printf("%2d ", (int) i);
		printf("\n");

		printf("(n=%3d): ", len); for (i = 0; i < len; i++) printf("%2x ", (int) buf[i]);
		printf("\n\n");

}


const char *
bus_str(int bus)
{
	switch (bus) {
	case BUS_USB: return "USB"; 
	case BUS_HIL: return "HIL"; 
	case BUS_BLUETOOTH: return "Bluetooth";
	case BUS_VIRTUAL: return "Virtual";
	default: return "Other";
	}
}



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

				fprintf(stderr, "   %s   permission denied\n", fname);
				continue;
			}

			/* Get Raw Name */
			int res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
			if (res < 0)
				perror("HIDIOCGRAWNAME");

			struct hidraw_devinfo info;
			res = ioctl(fd, HIDIOCGRAWINFO, &info);

			fprintf(stderr, "   %s   id %04hx:%04hx  name %s\n", fname, (unsigned int)info.vendor, (unsigned int)info.product, buf);
		}
}


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



int open_device(char *dev)
{
	char buf[256];
	int res, desc_size, i;

	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;

    char fname[256];
	sprintf(fname,"/dev/hidraw%s", dev);

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
		printf("bustype: %d (%s)", info.bustype, bus_str(info.bustype));
		printf(" vendor: 0x%04hx", info.vendor);
		printf(" product: 0x%04hx\n", info.product);
	}

	printf("\n\n");
	fflush(stdout);
}

int read_from_hid(char *buf, int len) {
    return read(fd, buf, len);
}
		