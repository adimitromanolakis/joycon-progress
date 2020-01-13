
OPT=-O2 -ggdb

default: hidraw-grip-initialization joycon


hidraw-grip-initialization: src/hidraw-grip-initialization.c
	gcc $(OPT) -o hidraw-grip-initialization src/hidraw-grip-initialization.c 

joycon: src/joycon-mouse.c src/udpclient.c src/uinput-virtual-mouse.c
	gcc $(OPT) -o joycon src/joycon-mouse.c src/udpclient.c src/uinput-virtual-mouse.c -lm


