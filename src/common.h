
#pragma once




extern int silent;
extern int debug_responses;
extern int report_type;

extern void hid_list();
extern uint8_t* hid_send_command(int cmd, unsigned char* data, int len);
extern int open_device(char* path);
extern int read_from_hid(char* buf, int len);
extern void close_hid_device();
