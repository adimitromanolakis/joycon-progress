#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include <stdint.h>

#include <windows.h>
#include <Winuser.h>


extern int silent;

/* emit function is identical to of the first example */
void emit(int fd, int type, int code, int val);

int uinput_fd;

int after_button_delay = 0;


int pG0 = 0, pG1 = 0, pG2 = 0;


// Button binding configuration
double mouse_speed = 1.2, mouse_expo = 1.2;
double wheel_move_amount = 1, wheel_move_fast_amount = 3;
uint32_t button_bindings[20];


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

float defered_move_x = 0, defered_move_y = 0;

inline float clamp(float value, float min, float max) {
    if(value>max) value = max;
    if(value<min) value = min;
    return value;
}

void uinput_update_v1(int G0,int G1,int G2, int A0, int A1, int A2)
{
    //A0=-A1;A1=-A1;A2=-A2;
    float moveX, moveY;
    //moveX = - (float)A2/150;
    //moveX += (float)A0/190 ;

    moveX = - (float)A2 / 130;
    moveX += (float)A0 / 350 ;

    moveY = (float)A1/80;

    moveX *= 1.3;
    moveY *= 1.4;

    float maxMove = 18*3;
    //float mult = 0.1;
    //moveX += 0.11;
      
    if(!silent) printf("Move: G1=%5d %5.2f %5.2f ", pG1, moveX,moveY);

    moveX *= 0.17 *1.3;
    moveY *= 0.19 *1.3;

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
     

     double exp = 1.1;

    if(moveX>0) {
        moveX = maxMove*pow(moveX/maxMove,exp);
    } else {
        moveX = -maxMove*pow(-moveX/maxMove,exp);
    }
     //exp=1;

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

/*debugging old version:
constants:

   0x0000000000402fac <+44>:    divss  0x8a8(%rip),%xmm1        # 0x40385c
130

  40304d:	f3 0f 5e 15 df 08 00 	divss  0x8df(%rip),%xmm2        # 403934 <CSWTCH.97+0x154>

290




 */

void uinput_update_1(int G0,int G1,int G2, int A0, int A1, int A2)
{
   //A0=-A1;A1=-A1;A2=-A2;
      float moveX;
      moveX = - (float)A2/130;
      moveX += (float)A0/290 ;
      
      float moveY = (float)A1/80;


      float maxMove = 18*3;
      //float mult = 0.1;
      //moveX += 0.11;
      
      if(!silent) printf("Move: G1=%5d %5.2f %5.2f ", pG1, moveX,moveY);

      moveX *= 0.17 *1.4;
      moveY *= 0.19 *1.4;

      moveX *= 1.2;
      moveY *= 1.2;

      moveX *= mouse_speed;
      moveY *= mouse_speed;


      moveX = clamp(moveX, -maxMove, maxMove);
      moveY = clamp(moveY, -maxMove, maxMove);

#define oo if(0)

     oo  if(A1 > 120) moveY += 0.3;
     oo  if(A1 < -120) moveY -= 0.3;

       if(A1 > 5320) moveY *= 1.2;
       if(A1 < -5320) moveY *= 1.2; 

    //if(A1 >   300) moveY +=1;
    //if(A1 <  -300) moveY -=1;
      
    oo  if(A2 >   400) moveX *= 1.1;
    oo  if(A2 <  -400) moveX *= 1.1;

      if(A2 >   950) moveX *=1.7;
      if(A2 <  -950) moveX *=1.7;
 

   

     // if (A2 > 6950) moveX *= 1.2;
     //      if (A2 < -6950) moveX *= 1.2;
      

     //printf("Move: %6d %6d %6d move = %5.2f %5.2f \n", A0, A1, A2, moveX, moveY);


     double exp = 1.14;
     exp = mouse_expo;

     if(moveX>0) {
        moveX = maxMove*pow(moveX/maxMove,exp);
     } else {
        moveX = -maxMove*pow(-moveX/maxMove,exp);
     }
    // exp=1;

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




#define BTN_LEFT 0x8001
#define BTN_RIGHT 0x8002


int button_state[10] = {0,0,0,0,0,0 };

void button_logic(int num, int state, int event) {
   state = state > 0 ? 1: 0;

   if(button_state[num] != state) {

      //printf("EMIT %d state=%d", event,state);

       INPUT  Input = { 0 };
       Input.type = INPUT_MOUSE;
       
       if (num == 1) {
           if (state == 1)
               Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
           else
               Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
       }

       if (num == 2) {
           if (state == 1)
               Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
           else
               Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
       }

       if (num > 3) {
           Input.type = INPUT_KEYBOARD;
           if (state == 1)
               Input.ki.dwFlags = 0;
           else
               Input.ki.dwFlags = KEYEVENTF_KEYUP;

           Input.ki.wScan = 0x45;

       }



       if (num == 5) Input.ki.wVk = VK_PRIOR;
       if (num == 6) {
           printf("NEXT KEY\n"); Input.ki.wVk = VK_NEXT;
       }


       if (num > 3) {
           Input.ki.wScan = MapVirtualKey(Input.ki.wVk, MAPVK_VK_TO_VSC);
           Input.ki.time = 0;
           Input.ki.dwExtraInfo = 0;
       }

       SendInput(1, &Input, sizeof(INPUT));


      //LINUX emit(uinput_fd, EV_KEY, event, state);
      //LINUX emit(uinput_fd, EV_SYN, SYN_REPORT, 0);

      if(num==1 || num==2)
      if(state==1) after_button_delay = 12;

      button_state[num] = state;
   }
}



void uinput_button_press(int num, int state)
{
   //printf(" BUT=%x\n", state);

   //button_logic(1, state & (0x40 | 0x80 | 0x2 | 0x20000 | 0x400000 | 0x800000 ), BTN_LEFT);
   //button_logic(2, state & ( 0x200 | 0x100 | 0x1 | 0x40000 ) , BTN_RIGHT);

    for (int i = 1;i <= 6;i++) {
        button_logic(i, state & button_bindings[i], i);
   }
}



int down_k = 0, up_k = 0;
int rpt_down = 0, rpt_up = 0;

int stick_down_count = 0, stick_up_count = 0;;



void scroll_up(int scroll_value)
{
   if(scroll_value < 0) {
      int distance = -1;

      if(scroll_value == -2)  {
         stick_up_count++;
         //if(stick_up_count>3) distance -= 1;
         //if(stick_up_count>7) 
         distance -= 2;
   
      } else stick_up_count = 0;


     //printf("WHEEL MOVE %d\n",distance );

      INPUT    Input = { 0 };
      Input.type = INPUT_MOUSE;
      Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
      Input.mi.mouseData = distance * 70;
      SendInput(1, &Input, sizeof(INPUT));
   } 

   if(scroll_value > 0) {
      int distance = 1; 

      if(scroll_value == 2)  {
         stick_down_count++;
         //if(stick_down_count>3) distance += 1;
         //if(stick_down_count>7) 
         distance += 2;
   
      } else stick_down_count = 0;

      //printf("WHEEL MOVE %d\n",distance );

      INPUT    Input = { 0 };
      Input.type = INPUT_MOUSE;
      Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
      Input.mi.mouseData = distance * 70;
      SendInput(1, &Input, sizeof(INPUT));

      }
}



void uinput_stick( int stick_horizontal, int stick_vertical)
{

   int scroll_value = 0;
   int rpt_interval = 0;

   if(stick_vertical < -200) { scroll_value = -1; rpt_interval = 6; }
   if(stick_vertical < -900) { scroll_value = -2; rpt_interval = 6 ; }
   if(stick_vertical > 200)  { scroll_value = 1; rpt_interval = 6; }
   if(stick_vertical > 900)  { scroll_value = 2; rpt_interval = 6; }

   if(!silent) 
      printf("STICK %d downk=%d rpt=%d   upk=%d rpt=%d counts=%d/%d scroll_value=%d\n",stick_vertical, down_k,rpt_down, up_k, rpt_up ,
   stick_down_count, stick_up_count,scroll_value);

   
   if(scroll_value < 0) {
      if(down_k==0) {

         if(!silent) printf("KEYDOWN ON ");
         //emit(uinput_fd, EV_KEY, KEY_DOWN, 1);
         scroll_up(scroll_value);

         down_k = 1;
         if(scroll_value == -1) rpt_up = rpt_interval; 
      } else {

         rpt_up--;
         if(rpt_up<=0) { 
            rpt_up = rpt_interval; 
            scroll_up(scroll_value);
         }
      }

   } else {

      if(down_k==1) {
         down_k = 0;
      }
   }

   if(scroll_value > 0) {
    
      if(up_k==0) {
         if(scroll_value == 1) scroll_up(scroll_value);

         up_k = 1;
         rpt_down = rpt_interval;
      } else {

         rpt_down--;
         if(rpt_down<=0) { 
            rpt_down = rpt_interval; 
            scroll_up(scroll_value);
         }

      }

   } else {
      //Repeat

      if(up_k==1) {
         up_k = 0;
      }
   }
}



void emit(int fd, int type, int code, int val)
{

}


int read_config_file();

void uinput_setup(char *serial_number)
{
    read_config_file();

}


void uinput_move(int dx, int dy)
{
      if(dx >  100) dx =  100;
      if(dy >  100) dy =  100;
      if(dx < -100) dx = -100;
      if(dy < -100) dy = -100;

      INPUT  Input = { 0 };
      Input.type = INPUT_MOUSE;
      Input.mi.dwFlags = MOUSEEVENTF_MOVE;
      Input.mi.dx = dx;
      Input.mi.dy = dy;
      SendInput(1, &Input, sizeof(INPUT));
     
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