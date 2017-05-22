#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>

#include <pigpio.h>

//Define RASPIO Pin Mappings
#define CLE 2 //Active High
#define ALE 3 // Active High
#define CE 4  // Active LOW
#define RE 17  // Active LOW
#define WE 27  // Active LOW
#define WP 22  // Active LOW
#define RB 10  // Busy or Active LOW


#define WE_PIN(a) ((a>>WE)&0x1)
#define CLE_PIN(a) ((a>>CLE)&0x1)
#define ALE_PIN(a) ((a>>ALE)&0x1)
#define CE_PIN(a) ((a>>CE)&0x1)
#define RE_PIN(a) ((a>>RE)&0x1)
#define WP_PIN(a) ((a>>WP)&0x1)



//////// Data Pins
#define IO0 14 // I/O 0 - RP J8 Pin 8
#define IO1 15 // I/O 0
#define IO2 18 // I/O 0
#define IO3 23 // I/O 0
#define IO4 24 // I/O 0
#define IO5 25 // I/O 0
#define IO6 8 // I/O 0
#define IO7 7 // I/O 0

#define IO0_PIN(a) ((a>>IO0)&0x1)
#define IO1_PIN(a) ((a>>IO1)&0x1)
#define IO2_PIN(a) ((a>>IO2)&0x1)
#define IO3_PIN(a) ((a>>IO3)&0x1)
#define IO4_PIN(a) ((a>>IO4)&0x1)
#define IO5_PIN(a) ((a>>IO5)&0x1)
#define IO6_PIN(a) ((a>>IO6)&0x1)
#define IO7_PIN(a) ((a>>IO7)&0x1)

uint8_t command = 0x00;
uint32_t column = 0x00000000;
uint32_t row = 0x00000000;
uint8_t buffer[2048+64];
uint32_t buffer_pointer = 0;


void init_pins()
{
  gpioSetMode(CLE,PI_INPUT);
  gpioSetMode(ALE,PI_INPUT);
  gpioSetMode(CE,PI_INPUT);
  gpioSetMode(WP,PI_INPUT);
  gpioSetMode(RE,PI_INPUT);
  gpioSetMode(WE,PI_INPUT);

  // Set 8 bit bus to INPUT
  gpioSetMode(IO0,PI_INPUT);
  gpioSetMode(IO1,PI_INPUT);
  gpioSetMode(IO2,PI_INPUT);
  gpioSetMode(IO3,PI_INPUT);
  gpioSetMode(IO4,PI_INPUT);
  gpioSetMode(IO5,PI_INPUT);
  gpioSetMode(IO6,PI_INPUT);
  gpioSetMode(IO7,PI_INPUT);

}

void RE_read(int gpio, int level, uint32_t tick)
{
  // This routine has to BE FAST!
  uint8_t cbuf;
  uint32_t values;
  // WE should check to make sure that CE is low
  values = gpioRead_Bits_0_31();  // Read all bits from port to get CLE,ALE, and IO

  if(level==0&&CE_PIN(values)==0) {
    // Host wants to read the buffer
    cbuf = buffer[buffer_pointer++];
    gpioWrite(IO0,0x01&cbuf);
    gpioWrite(IO1,0x02&cbuf);
    gpioWrite(IO2,0x04&cbuf);
    gpioWrite(IO3,0x08&cbuf);
    gpioWrite(IO4,0x10&cbuf);
    gpioWrite(IO5,0x20&cbuf);
    gpioWrite(IO6,0x40&cbuf);
    gpioWrite(IO7,0x80&cbuf);
  } else if (level==1) {
    // We can stop sending data on bus now
    gpioSetMode(IO0,PI_INPUT);
    gpioSetMode(IO1,PI_INPUT);
    gpioSetMode(IO2,PI_INPUT);
    gpioSetMode(IO3,PI_INPUT);
    gpioSetMode(IO4,PI_INPUT);
    gpioSetMode(IO5,PI_INPUT);
    gpioSetMode(IO6,PI_INPUT);
    gpioSetMode(IO7,PI_INPUT);
  }  

}

void do_work()
{
  // Do Some really cool stuff here
}

void NAND_read(int gpio, int level, uint32_t tick)
{
  uint32_t values; uint8_t rbyte=0x0;
  uint32_t x;
  // WE Pin goes from low to high , lets read values and set busy functions

  values = gpioRead_Bits_0_31();  // Read all bits from port to get CLE,ALE, and IO

  // Values should be, WE  - transition to high, CE-low, and either ALE or CLE high
  if((WE_PIN(values)==1)&&(level==1)&&(CE_PIN(values)==1)) {
    // were being addressed, lets respond now

    if(CLE_PIN(values)==1) {  // This is a command write
      // Lets read the command
      rbyte = IO0_PIN(values)|(IO1_PIN(values)<<1)|(IO2_PIN(values)<<2)|(IO3_PIN(values)<<3)|(IO4_PIN(values)<<4)
	|(IO5_PIN(values)<<5)|(IO6_PIN(values)<<6)|(IO7_PIN(values)<<7);
      command = rbyte;
      buffer_pointer = 0x0;
    }
    if(ALE_PIN(values)==1) { // This is an address write
      rbyte = IO0_PIN(values)|(IO1_PIN(values)<<1)|(IO2_PIN(values)<<2)|(IO3_PIN(values)<<3)|(IO4_PIN(values)<<4)
	|(IO5_PIN(values)<<5)|(IO6_PIN(values)<<6)|(IO7_PIN(values)<<7);
      // Lets figure out what to do with the byte from the command register
      buffer[buffer_pointer++] = rbyte;
    }

    // Operations for NAND Flash interface

    if(command==0x90&&buffer_pointer==0x01&&buffer[0]==0x00) {
      // READ ID Operation
      buffer[0] = 0xc8;  // Maker Code
      buffer[1] = 0xDA;  // Device Code
      buffer[2] = 0x90;  // 
      buffer[3] = 0x95;
      buffer[4] = 0x44;
      buffer_pointer=0x0;
      printf("Reading ID...\n");
    }


    if(command==0xFF) {
      // RESET
      // At initial startup, lets pull RB low to state that we are NOT ready
      gpioSetMode(RB,PI_OUTPUT);  gpioWrite(RB, 0);  // Set R/B to low, to show we are busy
      buffer_pointer=0x0;
      command = 0x00;
      init_pins();
      // were ready to take commands
      gpioSetPullUpDown(RB, PI_PUD_UP);
      gpioSetMode(RB,PI_INPUT);   // Set RB as float, which will pull this HIGH
    }

    if(command==0x30&&buffer_pointer==0x05) {
      // READ Operation

      // First, lets set the busy lines
      gpioSetMode(RB,PI_OUTPUT);  gpioWrite(RB, 0);  // Set R/B to low, to show we are busy

      for(x=0;x<=2112;x++) {
	buffer[x] = (((uint8_t)x)%64)+0x64;
      }
      buffer[0] = 'D';
      buffer[1] = 'o';
      buffer[2] = 'n';
      buffer[3] = 'W';
      buffer[4] = 'u';
      buffer[5] = 'z';
      buffer[6] = 'h';
      buffer[7] = 'e';
      buffer[8] = 'r';
      buffer[9] = 'e';
      // were ready to take commands
      gpioSetPullUpDown(RB, PI_PUD_UP);
      gpioSetMode(RB,PI_INPUT);   // Set RB as float, which will pull this HIGH
    }


  }
}

void intHandler(int dummy) {
  gpioTerminate();
  exit(0);
}

int main() {
  if(gpioInitialise() < 0) {
    printf("Error in init\n");
    exit(1);
  } else {
    printf("NAND Simulator Initialized\n");
  }

  // At initial startup, lets pull RB low to state that we are NOT ready
  gpioSetMode(RB,PI_OUTPUT);  gpioWrite(RB, 0);  // Set R/B to low, to show we are busy

  init_pins(); // Initialize input/output pins for Open NAND Flash Interface (ONFi)


  // Set ISR Functipn for WE Pin.
  gpioSetISRFunc(WE, RISING_EDGE,0,NAND_read);


  // Set ISR Functipn for RE Pin.
  gpioSetISRFunc(RE, EITHER_EDGE,0,RE_read);
  

  // were ready to take commands

  gpioSetPullUpDown(RB, PI_PUD_UP);
  gpioSetMode(RB,PI_INPUT);   // Set RB as float, which will pull this HIGH


  signal(SIGINT, intHandler);
  // Now loop while waiting
  while(1);
  return(0);
}
