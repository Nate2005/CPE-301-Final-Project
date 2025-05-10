// CPE 301 Final Project
// Author: Nathan Biggs
// Date: 5/9/2025

#include <LiquidCrystal.h>
#include <DHT11.h>
#include <Stepper.h>

/* Setting Buad Rate */
volatile unsigned char *myUCSR0A = (unsigned char *)0xC0;
volatile unsigned char *myUCSR0B = (unsigned char *)0xC1;
volatile unsigned char *myUCSR0C = (unsigned char *)0xC2;
volatile unsigned int *myUBRR0 = (unsigned int *)0xC4;

/* Serial Output */
#define TBE 0x20
volatile unsigned char *myUDR0 = (unsigned char *)0xC6;

/* Timer */
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned char *myTIFR1 = (unsigned char *)0x36;
volatile unsigned int *myTCNT1 = (unsigned int *)0x84;
const int ticks = (0.5 * (1.0 / (double)5)) / 0.0000000625;

/* ADC */
volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

/* On-Off Button (PK2) */
volatile unsigned char *port_k = (unsigned char *)0x108;
volatile unsigned char *ddr_k = (unsigned char *)0x107;
volatile unsigned char *pin_k = (unsigned char *)0x106;

/* 
Green LED (PE5)
Red LED (PE3)
Blue LED (PE4)
Reset Button (PE1)
*/
volatile unsigned char *port_e = (unsigned char *)0x2E;
volatile unsigned char *ddr_e = (unsigned char *)0x2D;
volatile unsigned char *pin_e = (unsigned char *)0x2C;

/* Yellow LED (PG5) */
volatile unsigned char *port_g = (unsigned char *)0x34;
volatile unsigned char *ddr_g = (unsigned char *)0x33;


/* LCD Setup */
const int RS = 11, EN = 10, D4 = 9, D5 = 8, D6 = 7, D7 = 6;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
bool updateLcd = true;
bool firstReading = true;
unsigned long previousMillis = 0;
const long interval = 60000;

/* Stepper Motor */
#define steps 1000
Stepper stepper(steps, 22, 28, 24, 26);
int previous = 0;


/* DHT11 Sensor */
DHT11 dht11(12);
static int temp = 0;
static int humidity = 0;

/* Water Senson */
static int lowLoopCount = 0;


// States Array: {Disabled (Y), Running (B), Error (R)} (Green on when Y and B are both off)
int states[3] = { 1, 0, 0 };

void setup() {
  U0init(9600);
  adc_init();

  /* Stepper Motor */
  stepper.setSpeed(30);

  /* LCD */
  lcd.begin(16, 2);

  /* Timer */
  *myTCCR1A = 0x00;
  *myTCCR1B = 0X00;
  *myTCCR1C = 0x00;
  *myTIFR1 |= 0x01;
  *myTIMSK1 |= 0x01;

  /* On-Off Button */
  *ddr_k &= 0xFB;
  *port_k |= 0x04;
  /* Reset Button */
  *ddr_e &= 0xFD;
  *port_e |= 0x02;

  /* Red LED */
  *ddr_e |= 0x08;
  /* Green LED */
  *ddr_e |= 0x20;
  /* Blue LED */
  *ddr_e |= 0x10;
  /* Yellow LED */
  *ddr_g |= 0x20;
}

void loop() {
  bool buttonState = *pin_k & 0x04;
  if (!buttonState) {
    states[0] = !states[0];
    delay(200);
  }

  if (states[0] == 1) {
    // Turning Y on, turning G, R, B off
    *port_g |= 0x20;
    *port_e &= 0xDF;
    *port_e &= 0xF7;
    *port_e &= 0xEF;
    // Clear LCD
    lcd.clear();
  } else {
    // Turning G on, turning Y off
    if (states[0] == 0 && states[1] == 0 && states[2] == 0) {
      *port_g &= 0xDF;
      *port_e |= 0x20;
    }

    // Stepper Motor
    int val = adc_read(0);
    int diff = val - previous;
    if (abs(diff) > 5) {
      stepper.step(diff);
      previous = val;
    }

    // Read Temp and Humidity
    if (firstReading) {
      temp = dht11.readTemperature();
      humidity = dht11.readHumidity();

      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temp);
      lcd.print(" C");

      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(humidity);
      lcd.print("%");
      firstReading = false;
    }

    int waterLvl = adc_read(1);
    if (waterLvl < 100) {
      lowLoopCount++;
      if (lowLoopCount >= 5) {
        *port_e &= 0xDF;
        *port_e |= 0x08;
        *port_e &= 0xEF;
        *port_g &= 0xDF;
        lcd.clear();
        lcd.print("Water Level Low");
        delay(200);
        states[2] = 1;
      }
    } else {
      lowLoopCount = 0;
      bool resetButtonState = *pin_e & 0x02;
      if (!resetButtonState) {
        if (states[1] == 0) {
          *port_e |= 0x20;
        } else {
          *port_e |= 0x10;
        }
        *port_e &= 0xF7;
        states[2] = 0;

        temp = dht11.readTemperature();
        humidity = dht11.readHumidity();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temp);
        lcd.print(" C");

        lcd.setCursor(0, 1);
        lcd.print("Humidity: ");
        lcd.print(humidity);
        lcd.print("%");

        delay(200);
      }
    }

    if (states[0] == 0 && states[2] == 0) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        if (updateLcd) {
          temp = dht11.readTemperature();
          humidity = dht11.readHumidity();

          lcd.setCursor(0, 0);
          lcd.print("Temp: ");
          lcd.print(temp);
          lcd.print(" C");

          lcd.setCursor(0, 1);
          lcd.print("Humidity: ");
          lcd.print(humidity);
          lcd.print("%");

          updateLcd = false;
        } else {
          updateLcd = true;
        }
      }
      if (temp > 50) {
        *port_e &= 0xDF;
        *port_e |= 0x10;
        states[1] = 1;
      } else {
        *port_g |= 0x20;
        *port_e &= 0xDF;
        *port_e &= 0xF7;
        *port_e &= 0xEF;
        states[1] = 0;
      }

    }
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

void adc_init() {
  // setup the A register
  *my_ADCSRA |= 0x80;
  *my_ADCSRA &= 0xBF;
  *my_ADCSRA &= 0xDF;
  *my_ADCSRA &= 0xF8;
  // setup the B register
  *my_ADCSRB &= 0xF7;
  *my_ADCSRB &= 0xF8;
  // setup the MUX Register
  *my_ADMUX &= 0x7F;
  *my_ADMUX |= 0x40;
  *my_ADMUX &= 0xDF;
  *my_ADMUX &= 0xF0;
}

unsigned int adc_read(unsigned char adc_channel_num) {
  *my_ADMUX &= 0xF0;
  *my_ADCSRB &= 0xF7;
  *my_ADMUX += adc_channel_num;
  *my_ADCSRA |= 0x40;
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  unsigned int val = *my_ADC_DATA & 0x03FF;
  return val;
}

// ISR(TIMER1_OVF_vect) {
//   // Stop the Timer
//   *myTCCR1B &= 0xF8;
//   // Load the Count
//   *myTCNT1 =  (unsigned int) (65535 -  (unsigned long) (ticks));
//   // Start the Timer
//   *myTCCR1B |= 0x01;
//   // if it's not the STOP amount
//   if(ticks != 65535)
//   {
//     // XOR to toggle PB6
//     *portB ^= 0x40;
//   }
// }