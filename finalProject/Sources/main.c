#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */



#define LCD_DATA PORTK
#define LCD_CTRL PORTK
#define RS 0x01
#define EN 0x02



unsigned int readNumber;
char messageMusicPlayer[] = "MUSIC PLAYER";
char messageTempSensor[]  = "TEMP SENSOR";
char messageLightSensor[] = "LIGHT SENSOR";
char messageKeypad[]      = "KEYPAD";

void COMWRT4(unsigned char);
void DATWRT4(unsigned char);
void MSDelay(unsigned int);       // 2.73 ms
void mSDelay(unsigned int itime);
void initialBuzzer(void);
void initialPORTH(void);
void initialSerialCom(void);

void lcdParameters(void);
void initialLCD(char *lcdMessage); 


void initialLightSensor(void);
void initialTempSensor(void);
void initialKeypad(void);

interrupt(((0x10000 - Vtimch5)/2)-1) void TC5_ISR(void);
interrupt(((0x10000-Vporth)/2)-1) void PORTH_ISR(void);
interrupt(((0x10000 - Vsci0)/2)-1) void SCI0_ISR(void);

void main(void) {
 
 
  unsigned int i=0;

  unsigned char startMessage[] = " 0. Light Sensor\r\n1. Music Player\r\n2. Temperature Sensor\r\n3. Keypad\r\n";
 
  DDRB = 0xFF;
  DDRP = 0xFF;
  
  
  initialPORTH();
  initialSerialCom();


  
  // Serial Communication Message
  while(startMessage[i] != '\0')  {  
   SCI0DRL = startMessage[i];
   while(!(SCI0SR1 & SCI0SR1_TDRE_MASK)); 
   ++i; 
  }
    
    
 
  // PTH is equal to zero, it will run light sensor
  if(PTH == 0) {
    initialLCD(messageLightSensor);   
    initialLightSensor();
  }
  
 
  // PTH is equal to one, it will run music player  
  if(PTH == 1) {               
    initialLCD(messageMusicPlayer);              
    initialBuzzer();
  }
  
  // PTH is equal to two, it will run temperature sendor  
  if(PTH == 2) {               
    initialLCD(messageTempSensor);              
    initialTempSensor();
  }
  
 
  if(PTH == 3) {               
    initialLCD(messageKeypad);              
    initialKeypad();
  }
   
  


  // Enable Interrupts 
  __asm(cli);
  
    for(;;) {
      __asm(wai);    // Wait to interrupts
    }
  
 
}



void lcdParameters(void) {
 
  DDRK = 0xFF;   
  COMWRT4(0x33);   //reset sequence provided by data sheet
  MSDelay(1);
  COMWRT4(0x32);   //reset sequence provided by data sheet
  MSDelay(1);
  COMWRT4(0x28);   //Function set to four bit data length
                                   //2 line, 5 x 7 dot format
  MSDelay(1);
  COMWRT4(0x06);  //entry mode set, increment, no shift
  MSDelay(1);

  COMWRT4(0x0E);  //Display set, disp on, cursor on, blink off
  MSDelay(1);
  COMWRT4(0x01);  //Clear display
  MSDelay(1);
  
  COMWRT4(0x80);  //set start posistion, home position
  MSDelay(1); 
  
}


void initialLCD(char *lcdMessage) {

  //unsigned char messageMusicPlayer[] = "MUSIC PLAYER";
  unsigned int i=0;
  
  lcdParameters();
  
  while(lcdMessage[i] != '\0') { 
    DATWRT4(lcdMessage[i]);
    MSDelay(1); 
    ++i; 
  }
    
  
}

// Light Sensor

void initialLightSensor(void) {


    unsigned int ledDance = 0x01;
    unsigned int i;


    DDRT = 0xFF;

    DDRB = 0xFF;    //PORTB as output
    DDRJ = 0xFF;    //PTJ as output for Dragon12+ LEDs
    PTJ=0x0;        //Allow the LEDs to dsiplay data on PORTB pins
    
    ATD0CTL2 = 0x80;     //Turn on ADC,..No Interrupt
    MSDelay(5);
    ATD0CTL3 = 0x08;  //one conversion, no FIFO
    ATD0CTL4 = 0xEB;  //8-bit resolu, 16-clock for 2nd phase,
                      //prescaler of 24 for Conversion Freq=1MHz  
    
    for(;;)
    {
    
      ATD0CTL5 = 0x84;  //Channel 4 (right justified, unsigned,single-conver,one chan only) 
      while(!(ATD0STAT0 & 0x80));
     
     
      // Sensor Value
      if(ATD0DR0L != 0 ) {
       
         for(i=0; i<8; ++i )  {
           PORTB = ledDance << i;
           MSDelay(2);
           
         }
                      
      }
     
      if(ATD0DR0L == 0 )   {
      
         PORTB = 0x00; 
         PTT = PTT ^0b00100000;
       
      }
      
  
    }
      
}




 // TEMPERATURE SENSOR 
void initialTempSensor(void) {
  
    DDRB = 0xFF;    //PORTB as output
    DDRJ = 0xFF;    //PTJ as output for Dragon12+ LEDs
    PTJ=0x0;        //Allow the LEDs to dsiplay data on PORTB pins
    
    ATD0CTL2 = 0x80;     //Turn on ADC,..No Interrupt
    MSDelay(5);
    ATD0CTL3 = 0x08;  //one conversion, no FIFO
    ATD0CTL4 = 0xEB;  //8-bit resolu, 16-clock for 2nd phase,
                      //prescaler of 24 for Conversion Freq=1MHz  
    for(;;)
    {
    ATD0CTL5 = 0x85;  //Channel 5 (right justified, unsigned,single-conver,one chan only) 

    while(!(ATD0STAT0 & 0x80));
    
    PORTB = ATD0DR0L;  //dump it on LEDs
    
    
    
    MSDelay(2);   //optional
    }
      
}



void COMWRT4(unsigned char command){

    unsigned char x;
    
    x = (command & 0xF0) >> 2;         //shift high nibble to center of byte for Pk5-Pk2
    LCD_DATA =LCD_DATA & ~0x3C;          //clear bits Pk5-Pk2
    LCD_DATA = LCD_DATA | x;          //sends high nibble to PORTK
    MSDelay(1);
    LCD_CTRL = LCD_CTRL & ~RS;         //set RS to command (RS=0)
    MSDelay(1);
    LCD_CTRL = LCD_CTRL | EN;          //rais enable
    MSDelay(3);
    LCD_CTRL = LCD_CTRL & ~EN;         //Drop enable to capture command
    MSDelay(5);                       //wait
    
    x = (command & 0x0F)<< 2;          // shift low nibble to center of byte for Pk5-Pk2
    LCD_DATA =LCD_DATA & ~0x3C;         //clear bits Pk5-Pk2
    LCD_DATA =LCD_DATA | x;             //send low nibble to PORTK
    LCD_CTRL = LCD_CTRL | EN;          //rais enable
    MSDelay(3);
    LCD_CTRL = LCD_CTRL & ~EN;         //drop enable to capture command
    MSDelay(5);

}


void DATWRT4(unsigned char data)
{
    unsigned char x;
  
    x = (data & 0xF0) >> 2;
    LCD_DATA =LCD_DATA & ~0x3C;                     
    LCD_DATA = LCD_DATA | x;
    MSDelay(1);
    LCD_CTRL = LCD_CTRL | RS;
    MSDelay(1);
    LCD_CTRL = LCD_CTRL | EN;
    MSDelay(1);
    LCD_CTRL = LCD_CTRL & ~EN;
    MSDelay(3);
   
    x = (data & 0x0F)<< 2;
    LCD_DATA =LCD_DATA & ~0x3C;                     
    LCD_DATA = LCD_DATA | x;
    LCD_CTRL = LCD_CTRL | EN;
    MSDelay(1);
    LCD_CTRL = LCD_CTRL & ~EN;
    MSDelay(5);

}

 
 void MSDelay(unsigned int itime)
 {
 
    unsigned int i;
    unsigned int j;
   
    TSCR1 = 0x80;
    TSCR2 = 0x00;
   
    for(i=0; i<itime; ++i) {
        for(j=0; j<1; ++j) { 
          TFLG2 = 0x80;                           // Clear TOF
          while(!(TFLG2 & TFLG2_TOF_MASK));       // Wait for overflow flag to be raised 
       
        }
    }  
 }



void initialBuzzer(void) 
{
  
 TSCR1 = 0x80;     // Timer Enable
 TSCR2 = 0x00;     // No prescale, no interrupt
 
 TIOS  = 0x20;     // Channel 5 to output compare
 TCTL1 = 0x04;     // PT5 toggle
 TIE   = 0x20;     // Interrupt Enable
 TC5   = TCNT;  
  
}

void initialSerialCom(void) 
{
  
  SCI0BDH = 0x00;
  SCI0BDL = 26;     // Baud Rate 9600
  SCI0CR1 = 0x00;
  SCI0CR2 = 0xAC;   // Enable Transmit, receive and interrupt
}


interrupt(((0x10000 - Vsci0)/2)-1) void SCI0_ISR(void) {

  // Convert HEX TO DECIMAL
  if(SCI0SR1 & SCI0SR1_RDRF_MASK)
    readNumber = SCI0DRL - '0';  
    
}



interrupt(((0x10000 - Vtimch5)/2)-1) void TC5_ISR(void) 
{
  
  
  if(readNumber == 0)  {               // 450 Hz
   
     TC5 = TC5 + 26671;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x3F;
     PTP   = 0x07; 
      
   }
  
  
  if(readNumber == 1)  {               //  500 Hz
                                      
   TC5 = TC5 + 24004;
   TFLG1 = TFLG1 | TFLG1_C5F_MASK;
   PORTB = 0x06;
   PTP   = 0x07; 
    
   }
  
  
  if(readNumber == 2)  {               //  550 Hz
   
     TC5 = TC5 + 21821;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x5B;
     PTP   = 0x07; 
      
   }
  
  
  if(readNumber == 3)  {               //  600 Hz
   
   TC5 = TC5 + 20003;
   TFLG1 = TFLG1 | TFLG1_C5F_MASK;
   PORTB = 0x4F;
   PTP   = 0x07; 
    
   }
  
   if(readNumber == 4)  {              //  650 Hz
   
     TC5 = TC5 + 18464;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x66;
     PTP   = 0x07; 
      
   }
  
  
   if(readNumber == 5)  {               //  700 Hz
   
     TC5 = TC5 + 17145;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x6D;
     PTP   = 0x07; 
    
   }
  
    
   if(readNumber == 6)  {                // 750 Hz
     
     TC5 = TC5 + 16002;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x7D;
     PTP   = 0x07; 
    
   }
  
  
  
   if(readNumber == 7)  {                // 800 Hz
   
     TC5 = TC5 + 15002;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x47;
     PTP   = 0x07; 
    
   }
   
   if(readNumber == 8)  {               // 850 Hz
   
     TC5 = TC5 + 14120;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x7F;
     PTP   = 0x07; 
    
   }
   
   
   if(readNumber == 9)  {               // 900 Hz
   
     TC5 = TC5 + 13335;
     TFLG1 = TFLG1 | TFLG1_C5F_MASK;
     PORTB = 0x6F;
     PTP   = 0x07; 
    
   }
     
    
}


void initialPORTH(void) 
{

  PIEH = 0x07;
  PPSH = 0x07; 
  
}

interrupt(((0x10000-Vporth)/2)-1) void PORTH_ISR(void)
{
  PIFH = 0x07;   
}


void initialKeypad(void){              


  const unsigned char keypad[4][4] =
    {
    '1','2','3','A',
    '4','5','6','B',
    '7','8','9','C',
    '*','0','#','D'
    };
    
    
  unsigned char column,row;
  unsigned int i;
              
   DDRB = 0xFF;                           //MAKE PORTB OUTPUT
   DDRJ |=0x02; 
   PTJ &=~0x02;                            //ACTIVATE LED ARRAY ON PORT B
   DDRP |=0x0F;                           //
   PTP |=0x0F;                            //TURN OFF 7SEG LED
   DDRA = 0x0F;                           //MAKE ROWS INPUT AND COLUMNS OUTPUT
   
 
   for(;;){                              //OPEN WHILE(1)
      do{                                 //OPEN do1
         PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
      }while(row == 0x00);                //WAIT UNTIL KEY PRESSED //CLOSE do1



      do{                                 //OPEN do2
         do{                              //OPEN do3
            mSDelay(1);                   //WAIT
            row = PORTA & 0xF0;           //READ ROWS
         }while(row == 0x00);             //CHECK FOR KEY PRESS //CLOSE do3
         
         mSDelay(15);                     //WAIT FOR DEBOUNCE
         row = PORTA & 0xF0;
      }while(row == 0x00);                //FALSE KEY PRESS //CLOSE do2

      for(;;){                           //OPEN while(1)
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x01;                   //COLUMN 0 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 0
            column = 0;
            break;                        //BREAK OUT OF while(1)
         }
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x02;                   //COLUMN 1 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 1
            column = 1;
            break;                        //BREAK OUT OF while(1)
         }

         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x04;                   //COLUMN 2 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 2
            column = 2;
            break;                        //BREAK OUT OF while(1)
         }
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x08;                   //COLUMN 3 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 3
            column = 3;
            break;                        //BREAK OUT OF while(1)
         }
         row = 0;                         //KEY NOT FOUND
      break;                              //step out of while(1) loop to not get stuck
      }                                   //end while(1)

      if(row == 0x10){
         // PORTB=keypad[0][column];         //OUTPUT TO PORTB LED
            for(i=0; i<8; ++i )  {
               PORTB = 0x80 >> i;
               MSDelay(2);
           
         }
         
      
      }
      else if(row == 0x20){
         //PORTB=keypad[1][column];
         for(i=0; i<8; ++i )  {
               PORTB = 0x01 << i;
               MSDelay(2);
         }
      }
      else if(row == 0x40){
         //PORTB=keypad[2][column];
         
         
          
            for(i=0; i<4; ++i )  {
                 PORTB = 0x08 >> i;
                 MSDelay(2);
                 PORTB = 0x10 << i;
                 MSDelay(2);
           }
         
      }
      else if(row == 0x80){
         //PORTB=keypad[3][column];
         
            for(i=0; i<4; ++i )  {
                 PORTB = 0x80 >> i;
                 MSDelay(2);
                 PORTB = 0x01 << i;
                 MSDelay(2);
           }
 
      }

      do{
         mSDelay(15);
         PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
      }while(row != 0x00);                //MAKE SURE BUTTON IS NOT STILL HELD
   }                                      //CLOSE WHILE(1)
}       



void mSDelay(unsigned int itime){
unsigned int i; unsigned int j;
   for(i=0;i<itime;i++)
      for(j=0;j<4000;j++);
}


