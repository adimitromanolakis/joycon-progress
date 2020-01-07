#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

extern int silent;

/* emit function is identical to of the first example */
void emit(int fd, int type, int code, int val);

int uinput_fd;
struct uinput_user_dev uidev;

int after_button_delay = 0;


int pG0=0, pG1=0, pG2=0;

void uinput_move(int dx, int dy);

void uinput_update_0(int G0,int G1,int G2, int A0, int A1, int A2);
void uinput_update_1(int G0,int G1,int G2, int A0, int A1, int A2);

void uinput_update(int G0,int G1,int G2, int A0, int A1, int A2) 
{
   uinput_update_1(G0,G1,G2,A0,A1,A2);
}


void uinput_update_0(int G0,int G1,int G2, int A0, int A1, int A2)
{

   if(after_button_delay > 0) {
      after_button_delay --;
      return;
   }

   //accell difference
   int D0 = G0-pG0;
   int D1 = G1-pG1;

   pG0 = G0;
   pG1 = G1;

   
   float moveX,moveY;

   moveX = (float)D0/25;
   moveY = (float)D1/25;
   
         //if(moveX>15) moveX= 15;
   //if(moveY>15) moveY= 15;

   printf("move=%.4f %.4f\n",moveX,moveY);
   

   uinput_move((int)(moveX+0.5), (int)(moveY+0.5));
  
}

int defered_move_x = 0, defered_move_y = 0;

inline float clamp(float value, float min, float max) {
   if(value>max) value = max;
   if(value<min) value = min;
   return value;
}

void uinput_update_1(int G0,int G1,int G2, int A0, int A1, int A2)
{
   //A0=-A1;A1=-A1;A2=-A2;
      float moveX;
      moveX = - (float)A2/150;
      moveX += (float)A0/190 ;
      
      float moveY = (float)A1/80;


      float maxMove = 18*3;
      //float mult = 0.1;
      //moveX += 0.11;
      
      if(!silent) printf("Move: G1=%5d %5.2f %5.2f ", pG1, moveX,moveY);

      moveX *= 0.17 *1.7;
      moveY *= 0.19 *1.7;

      moveX = clamp(moveX, -maxMove, maxMove);
      moveY = clamp(moveY, -maxMove, maxMove);

#define oo if(0)

     oo  if(A1 > 120) moveY += 0.3;
     oo  if(A1 < -120) moveY -= 0.3;

     oo  if(A2 > 320) moveX -= 0.3;
     oo   if(A2 < -320) moveX += 0.3   ;



      //if(A1 >   300) moveY +=1;
      //if(A1 <  -300) moveY -=1;
      
    oo  if(A2 >   400) moveX *= 1.1;
    oo  if(A2 <  -400) moveX *= 1.1;

      if(A2 >   950) moveX *=1.7;
      if(A2 <  -950) moveX *=1.7;
 

     //if(moveX>0.2) moveX +=0.7;
     //if(moveX< -0.2) moveX-=0.7;
     

     double exp = 0.99;

     if(moveX>0) {
        moveX = maxMove*pow(moveX/maxMove,exp);
     } else {
        moveX = -maxMove*pow(-moveX/maxMove,exp);
     }
     exp=1;

     if(moveY>0) {
        moveY = maxMove*pow(moveY/maxMove,exp);
     } else {
        moveY = -maxMove*pow(-moveY/maxMove,exp);
     }


    // printf("move=%10.4f %10.4f\n",moveX,moveY);
     
      if(after_button_delay > 0) {
         defered_move_x += moveX;
         defered_move_y += moveY;
         after_button_delay --;
         return;
      }


      if(defered_move_x>0 || defered_move_y>0) {
            
            uinput_move((int)(defered_move_x+0.5), (int)(defered_move_x+0.5));
            defered_move_x = 0;
            defered_move_y = 0;

      }

    if(!silent)  printf(" =>%5.2f %5.2f ", moveX,moveY);

     // if(abs(moveX)<0.4) moveX =0;

      uinput_move((int) ( 0.5 + moveX) , (int) (0.5+moveY));
  
}


int button_state[10] = {0,0,0,0,0,0 };

void button_logic(int num, int state, int event) {
   state = state>0 ? 1: 0;

   if(button_state[num] != state) {

      //printf("EMIT %d state=%d", event,state);

      emit(uinput_fd, EV_KEY, event, state);
      emit(uinput_fd, EV_SYN, SYN_REPORT, 0);

      if(state==1) after_button_delay = 45/3;

      button_state[num] = state;
   }
}

void uinput_button_press(int num, int state)
{
   // printf(" BUT=%x\n", state);

   button_logic(1, state & (0x40 | 0x80 | 0x2 | 0x20000 | 0x400000 | 0x800000 ), BTN_LEFT);
   button_logic(2, state & ( 0x200 | 0x100 ) , BTN_RIGHT);
}

int down_k = 0, up_k = 0;
int rpt_down = 0, rpt_up = 0;

int stick_down_count = 0, stick_up_count = 0;;

void scroll_up(int stick_vertical)
{
   if(stick_vertical < -200) {
      int distance = -1;

      if(stick_vertical < -900)  {
         stick_up_count++;
         //if(stick_up_count>3) distance -= 1;
         //if(stick_up_count>7) 
         distance -= 3;
   
      } else stick_up_count = 0;


      emit(uinput_fd, EV_REL, REL_WHEEL, distance);
      printf("WHEEL MOVE %d\n",distance );
      emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
   } 

   if(stick_vertical > 200) {
      int distance = 1; 

      if(stick_vertical > 900)  {
         stick_down_count++;
         //if(stick_down_count>3) distance += 1;
         //if(stick_down_count>7) 
         distance += 3;
   
      } else stick_down_count = 0;

      emit(uinput_fd, EV_REL, REL_WHEEL, distance);   
      printf("WHEEL MOVE %d\n",distance );
      emit(uinput_fd, EV_SYN, SYN_REPORT, 0);

      }
       

}


void uinput_stick( int stick_horizontal, int stick_vertical)
{
   if(!silent) printf("STICK %d downk=%d rpt=%d   upk=%d rpt=%d counts=%d/%d ",stick_vertical, down_k,rpt_down, up_k, rpt_up ,
   stick_down_count, stick_up_count);
   

   if(stick_vertical < -200) {
      if(down_k==0) {

         if(!silent) printf("KEYDOWN ON ");
         //emit(uinput_fd, EV_KEY, KEY_DOWN, 1);
         scroll_up(stick_vertical);

         down_k = 1;
         rpt_up = 5; 
      } else {

         rpt_up--;
         if(rpt_up<=0) { 
            rpt_up = 5; 
            scroll_up(stick_vertical);
         }
      }

   } else {

      if(down_k==1) {
         if(!silent) printf("KEYDOWN OFF ");
         down_k = 0;
      }
   }

   if(stick_vertical > 200) {
    
      if(up_k==0) {
         //emit(uinput_fd, EV_KEY, KEY_UP, 1);
         scroll_up(stick_vertical);

         up_k = 1;
         rpt_down = 5;
      } else {

         rpt_down--;
         if(rpt_down<=0) { 
            rpt_down = 5; 
            scroll_up(stick_vertical);
         }

      }


  

   } else {
      //Repeat

      if(up_k==1) {
         //emit(uinput_fd, EV_KEY, KEY_UP, 0);
         //emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
         up_k = 0;
      }
   }




}




void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   int ret = write(fd, &ie, sizeof(ie));
}



void uinput_setup()
{
   int i = 50;

   uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

   /* enable mouse button left and relative events */
   ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
   ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
   ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
   ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);

   ioctl(uinput_fd, UI_SET_KEYBIT, KEY_UP);
   ioctl(uinput_fd, UI_SET_KEYBIT, KEY_DOWN);
   

   ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
   ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
   ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
   ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);



    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    if(write(uinput_fd, &uidev, sizeof(uidev)) < 0)
        printf("error: write");

   ioctl(uinput_fd, UI_DEV_CREATE);
}

void uinput_move(int dx, int dy)
{
      if(dx>100) dx = 100;
      if(dy>100) dy = 100;
      if(dx < -100) dx = -100;
      if(dy< -100) dy = -100;
      
      emit(uinput_fd, EV_REL, REL_X, dx);
      emit(uinput_fd, EV_REL, REL_Y, dy);
      emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
}


#ifdef MAIN
int main(void)
{
   //struct uinput_setup usetup;
   int i;

   uinput_setup();

   #if 0
   memset(&usetup, 0, sizeof(usetup));
   usetup.id.bustype = BUS_USB;
   usetup.id.vendor = 0x1234; /* sample vendor */
   usetup.id.product = 0x5678; /* sample product */
   strcpy(usetup.name, "Example device");

   ioctl(fd, UI_DEV_SETUP, &usetup); 
   #endif


   /*
    * On UI_DEV_CREATE the kernel will create the device node for this
    * device. We are inserting a pause here so that userspace has time
    * to detect, initialize the new device, and can start listening to
    * the event, otherwise it will not notice the event we are about
    * to send. This pause is only needed in our example code!
    */
   sleep(1);

   i=50;

   /* Move the mouse diagonally, 5 units per axis */
   while (i--) {
      emit(fd, EV_REL, REL_X, 5);
      emit(fd, EV_REL, REL_Y, 5);
      emit(fd, EV_SYN, SYN_REPORT, 0);
      usleep(15000);
   }

   /*
    * Give userspace some time to read the events before we destroy the
    * device with UI_DEV_DESTOY.
    */
   sleep(1);

   ioctl(fd, UI_DEV_DESTROY);
   close(fd);

   return 0;
}
#endif