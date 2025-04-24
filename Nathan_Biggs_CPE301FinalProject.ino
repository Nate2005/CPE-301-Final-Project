// CPE 301 Final Project
// Author: Nathan Biggs
// Date: 5/9/2025

#include <LiquidCrystal.h>

/* Setting Buad Rate */
volatile unsigned char *myUCSR0A = (unsigned char *)0xC0;
volatile unsigned char *myUCSR0B = (unsigned char *)0xC1;
volatile unsigned char *myUCSR0C = (unsigned char *)0xC2;
volatile unsigned int *myUBRR0 = (unsigned int *)0xC4;

/* Serial Output */
#define TBE 0x20
volatile unsigned char *myUDR0 = (unsigned char *) 0xC6;

/* On-Off Button (PK2) */
volatile unsigned char *port_k = (unsigned char *)0x108;
volatile unsigned char *ddr_k = (unsigned char *)0x107;
volatile unsigned char *pin_k = (unsigned char *)0x106;

/* 
Green LED (PE5)
Red LED (PE3)
Blue LED (PE4)
*/
volatile unsigned char *port_e = (unsigned char *)0x2E;
volatile unsigned char *ddr_e = (unsigned char *)0x2D;

/* Yellow LED (PG5) */
volatile unsigned char *port_g = (unsigned char *)0x34;
volatile unsigned char *ddr_g = (unsigned char *)0x33;

/* LCD Setup */
const int RS = 11, EN = 10, D4 = 9, D5 = 8, D6 = 7, D7 = 6;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// States Array: {Disabled (Y), Running (B), Error (R)} (Green on when Y and B are both off)
int states[3] = {1, 0, 0};

void setup() {
  U0init(9600);

  /* LCD */
  lcd.begin(16, 2);

  /* On-Off Button */
  *ddr_k &= 0xFB;
  *port_k |= 0x04;

  /* Red LED */
  *ddr_e &= 0xF7;
  /* Green LED */
  *ddr_e &= 0xDF;
  /* Blue LED */
  *ddr_e &= 0xEF;
  /* Yellow LED */
  *ddr_g &= 0xDF;
}

void loop() {
  if (!(*pin_k && 0x04 == 0x04) && states[0] == 1) {
    // Turning Y on, turning G, R, B off
    states[0] = 0;
    *port_g |= 0x20;
    *port_e &= 0xDF;
    *port_e &= 0xF7;
    *port_e &= 0xEF;
    
  } else if (!(*pin_k && 0x04 == 0x04) && states[0] == 0) {
    // Turning G on, turning Y off
    states[0] = 1;
    *port_g &= 0xDF;
    *port_e |= 0x20;

    
  }
}

void U0init(int U0baud) {
  unsigned long fcpu = 16000000;
  unsigned int tbaud;

  tbaud = (fcpu / 16 / U0baud - 1);
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

void putChar(unsigned char U0pdata) {
  while (!(*myUCSR0A & TBE));
  *myUDR0 = U0pdata;
}
