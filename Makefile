
OPT=-O2 -ggdb

default: hidraw-grip-initialization joycon


hidraw-grip-initialization: src/hidraw-grip-initialization.c
	gcc $(OPT) -o hidraw-grip-initialization src/hidraw-grip-initialization.c 

API=hidraw

ifeq ($(API),hidraw)

  DEVICE=src/device-hidraw.c

else

  DEVICE=src/device-hidapi.c
  OPT += -I/usr/local/include/hidapi/
  LIBS=-lhidapi-libusb
  LIBS=-lhidapi-hidraw

endif


SRC=src/joycon-mouse.c src/udpclient.c src/uinput-virtual-mouse.c
SRC+=$(DEVICE)


joycon: $(SRC)
	gcc $(OPT) -o joycon $(SRC) -lm $(LIBS)


hid1: hid1.c
	gcc $(OPT) -o hid1 hid1.c -L/usr/local/lib $(LIBS)