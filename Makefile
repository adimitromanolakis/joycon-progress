default: hidraw-grip-initialization hidraw


hidraw-grip-initialization: hidraw-grip-initialization.c
	gcc -O2 -o hidraw-grip-initialization hidraw-grip-initialization.c

hidraw:  udpclient.c hid-example.c
	gcc -O2 -o hidraw  udpclient.c hid-example.c


