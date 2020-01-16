#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h> 

//#include "slre/slre.h"
//#include "slre/slre.c"

//struct cap matches[4 + 1];





 #define QQ(a,b) if( strcmp(arg,a) == 0 ) return b;
uint32_t string_map_joycon(char *arg) {
    
    QQ("JOYCON_X",  0x020000)
    QQ("JOYCON_Y",  0x010000)
    QQ("JOYCON_A",  0x080000)
    QQ("JOYCON_B",  0x040000)
    QQ("JOYCON_R",  0x400000)    
    QQ("JOYCON_ZR", 0x800000)
    
    QQ("JOYCON_UP",    0x02)
    QQ("JOYCON_DOWN",  0x01)
    QQ("JOYCON_LEFT",  0x08)
    QQ("JOYCON_RIGHT", 0x04)

    QQ("JOYCON_MINUS", 0x100)
    QQ("JOYCON_PLUS",  0x200)

    QQ("JOYCON_L",  0x40)
    QQ("JOYCON_ZL", 0x80)

    QQ("JOYCON_CAPTURE", 0x2000)
    QQ("JOYCON_HOME",    0x1000)

    QQ("JOYCON_RAIL_RIGHT_TOP", 0x100000)
    QQ("JOYCON_RAIL_RIGHT_BOTTOM", 0x200000)

    QQ("JOYCON_RAIL_LEFT_TOP", 0x20)
    QQ("JOYCON_RAIL_LEFT_BOTTOM", 0x10)
    
    return -1;
}


int string_map_virtual(char *arg) {    
    QQ("MOUSE_LEFT", 1)
    QQ("MOUSE_RIGHT", 2)
    QQ("KEY_UP",  3)
    QQ("KEY_DOWN",  4)
    QQ("KEY_PAGEUP",  5)    
    QQ("KEY_PAGEDOWN", 6)
    
    return -1;

}
#undef QQ


extern double mouse_speed;
extern double mouse_expo;
extern double wheel_move_amount;
extern double wheel_move_fast_amount;
extern uint32_t button_bindings[20];


char str[3][200];

int spc(char c) { return c == '\t' || c== ' '; }

int matcher(char *buf, int len)
{
    char *p;
    int i = 0;

    while(i < len && spc(buf[i])) i++;

    p = str[0];
    while(i < len && !spc(buf[i]) && buf[i] != '=') {
        p[0] = buf[i]; p++; i++; 
    }
    p[0] = 0;

    while(i < len && spc(buf[i])) i++;
    //while(i < len && buf[i] == '=') i++;
    //while(i < len && spc(buf[i])) i++;

    p = str[1]; 
    while(i < len && !spc(buf[i])) { p[0] = buf[i]; p++; i++; } 
    p[0] = 0;

    while(i < len && spc(buf[i])) i++;

    p = str[2]; 
    while(i < len && !spc(buf[i])) { p[0] = buf[i]; p++; i++; } 
    p[0] = 0;

    return 1;
}

void
get_config_file_name(char *buf)
{

    


}

char config_file[256] = "config.txt";

int
read_config_file()
{
    FILE* f;
    #ifdef __linux__
    f = fopen(config_file, "r");
    #else
    fopen_s(&f, config_file, "r");
    #endif

    if(!f) {
        fprintf(stderr,"Cannot open configuration file: %s", config_file);
        exit(1);
    }

    char *fgets(char *s, int size, FILE *stream);
    char buf[256];

    for(int i=0;i<20;i++) button_bindings[i] = 0;

    while (fgets(buf, 256, f) != NULL)
    {
        for(int i=0;i<strlen(buf);i++) buf[i] = toupper(buf[i]);
        for(int i=0;i<strlen(buf);i++) if(buf[i] == '#') buf[i] = 0;
        
        if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = 0;

        int ret = matcher(buf,(int)strlen(buf));

        if(0) printf("MY  /%s/   /%s/ /%s/ %d %d\n", str[0],str[1],str[2],
            string_map_virtual(str[1]),
            string_map_joycon(str[2])
        );

        if(strcmp(str[0],"MOUSE_SPEED") == 0) {
            mouse_speed = atof(str[1]);
            printf("CONFIG: set mouse speed = %f\n",mouse_speed);
        }

        if(strcmp(str[0],"MOUSE_EXPO") == 0) {
            mouse_expo = atof(str[1]);
            printf("CONFIG: set mouse_expo = %f\n",mouse_expo);
        }

        if(strcmp(str[0],"BIND") == 0) {
            int virtual = string_map_virtual(str[1]);
            uint32_t joycon_button = string_map_joycon(str[2]);

            printf("CONFIG: add binding to %d -> %x\n", virtual,joycon_button);
            button_bindings[virtual] += joycon_button;
        }

    }
    for(int i=0;i<20;i++) {
        printf("BUTTON BIDING: %d -> %06x\n",i, button_bindings[i]);
    }

    //exit(0);

    return 0;
}

