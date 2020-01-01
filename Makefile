default: hidraw-grip-initialization hidraw


hidraw-grip-initialization: hidraw-grip-initialization.c
	gcc -O2 -o hidraw-grip-initialization hidraw-grip-initialization.c 

hidraw:  udpclient.c hid-example.c  uinput-virtual-mouse.c
	gcc -O2 -o hidraw  udpclient.c hid-example.c uinput-virtual-mouse.c -lm


