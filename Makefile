
OPT=-O2 -ggdb


API=hidraw

ifeq ($(API),hidraw)

  DEVICE = src/device-hidraw.c
  VIRTUAL_MOUSE = src/virtual-mouse-uinput.c

else

  DEVICE = src/device-hidapi.c
  VIRTUAL_MOUSE = src/virtual-mouse-windows.c
  VIRTUAL_MOUSE = src/virtual-mouse-uinput.c

  OPT += -I/usr/local/include/hidapi/
  LIBS = -lhidapi-libusb
  LIBS = -lhidapi-hidraw

endif


SRC=src/joycon-mouse.c src/udpclient.c src/configuration.c
SRC+=$(DEVICE)
SRC+=$(VIRTUAL_MOUSE)

default: hidraw-grip-initialization joycon

hidraw-grip-initialization: src/hidraw-grip-initialization.c
	gcc $(OPT) -o hidraw-grip-initialization src/hidraw-grip-initialization.c 

joycon: $(SRC)
	gcc $(OPT) -o joycon $(SRC) -lm $(LIBS)


hid1: hid1.c
	gcc $(OPT) -o hid1 hid1.c -L/usr/local/lib $(LIBS)