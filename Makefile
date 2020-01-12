default: hidraw-grip-initialization joycon


hidraw-grip-initialization: hidraw-grip-initialization.c
	gcc -O2 -o hidraw-grip-initialization hidraw-grip-initialization.c 

joycon: joycon-mouse.c udpclient.c uinput-virtual-mouse.c
	gcc -ggdb -O2 -o joycon joycon-mouse.c udpclient.c uinput-virtual-mouse.c -lm


