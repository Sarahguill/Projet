#include <msp430.h> 
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define CMDLEN  12               // longueur maximum de la commande utilisateur
#define LF      0x0A
#define CR      0x0D

#define MCU_CLOCK           1100000
#define PWM_FREQUENCY       46      // In Hertz, ideally 50Hz.

#define SERVO_STEPS         180     // Maximum amount of steps in degrees (180 is common)
#define SERVO_MIN           700     // The minimum duty cycle for this servo
#define SERVO_MAX           3000    // The maximum duty cycle

unsigned int PWM_Period     = (MCU_CLOCK / PWM_FREQUENCY);  // PWM Period
unsigned int PWM_Duty       = 5;                            // %


/***********************************
 * Appel aux fonctions
 ***********************************/
void InitUART(void);
void RXdata(unsigned char *c);
void TXdata(unsigned char c);
void Send_STR_UART(const char *msg);
void command( char *cmd );
void InitPWM(void);
void delay(void);


/***********************************
 * Le main
 ***********************************/
void main(void)
{
  unsigned char c;
  char  cmd[CMDLEN];      // tableau de caractere lie a la commande user
  int   nb_car;           // compteur nombre carateres saisis

    WDTCTL = WDTPW + WDTHOLD;   // Stop WDT
    // clock calibration verification
    if(CALBC1_1MHZ==0xFF || CALDCO_1MHZ==0xFF)
      __low_power_mode_4();
    // factory calibration parameters
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    InitPWM();
    InitUART();

    nb_car = 0;
    Send_STR_UART("MSP430 Ready !");
    while(1)
    {
      if( nb_car<(CMDLEN-1) )
      {
        RXdata(&c);
        if( (cmd[nb_car]=c) != CR )
        {
        TXdata(c);
        nb_car++;
        }
        else
        {
          cmd[nb_car]=0x00;
          command(cmd);
          nb_car=0;
        }
      }
    }
}



/***********************************
 * Les fonctions
 ***********************************/

void InitUART(void)
{
    P1SEL |= (BIT1 + BIT2);                 // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= (BIT1 + BIT2);                // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                   // SMCLK
    UCA0BR0 = 104;                          // 1MHz, 9600
    UCA0BR1 = 0;                            // 1MHz, 9600
    UCA0CTL0 &= ~UCPEN & ~UCPAR & ~UCMSB;
    UCA0CTL0 &= ~UC7BIT & ~UCSPB & ~UCMODE1;
    UCA0CTL0 &= ~UCMODE0 & ~UCSYNC;
    UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**
}

void RXdata(unsigned char *c)
{
    while (!(IFG2&UCA0RXIFG));              // buffer Rx USCI_A0 plein ?
    *c = UCA0RXBUF;
}

void TXdata( unsigned char c )
{
    while (!(IFG2&UCA0TXIFG));              // buffer Tx USCI_A0 vide ?
    UCA0TXBUF = c;
}

void Send_STR_UART(const char *msg)
{
    int i = 0;
    for(i=0 ; msg[i] != 0x00 ; i++)
    {
        TXdata(msg[i]);
    }
}

void InitPWM(void)
{
    P1DIR |= BIT6;                          // P1.6/TA0.1 is used for PWM, thus also an output -> servo 1
    P1DIR &= ~ BIT1;
    P1OUT = 0;                              // Clear all outputs P1

    P1SEL |= BIT6;                          // P1.6 select TA0.1 option

    // if SMCLK is about 1MHz (or 1000000Hz),
    // and 1000ms are the equivalent of 1 Hz,
    // then, by setting CCR0 to 20000 (1000000 / 1000 * 20)
    // we get a period of 20ms
    TA0CCR0 = 20000-1;                           // PWM Period TA0.1

    // setting 1500 is 1.5ms is 0deg. servo pos
    TA0CCR1 = 1500;                            // CCR1 PWM duty cycle
    TA0CCTL1 = OUTMOD_7;                       // CCR1 reset/set
    TA0CTL   = TASSEL_2 + MC_1;                // SMCLK, up mode
}

void delay(void)
{
    volatile unsigned long i;
    i = 49999;
    do (i--);
    while (i != 0);
}

void command( char *cmd )
{
    if(strcmp(cmd, "H") == 0)          // aide
      {
        // PWM : Go to 0°
        TA0CCR1 = 1000;
        delay();

        Send_STR_UART("\r\nAide : commandes existantes :\n");
        Send_STR_UART("\r\n'O' : Ouverture Serre\n");
        Send_STR_UART("\r\n'F' : Fermeture Serre\n");
        Send_STR_UART("\r\n'H' : Help\n");
      }
    else if (strcmp(cmd, "O") == 0)     // ouverture
    {

        // Go to 180°
        TA0CCR1 = 2000;
        delay();

        Send_STR_UART("\rSerre ouverte ! \n");
    }
    else if (strcmp(cmd, "F") == 0)     // fermeture
    {
        // PWM : Go to 0°
        TA0CCR1 = 1000;
        delay();

        Send_STR_UART("\rSerre fermee ! \n");
    }
    else
    {
      Send_STR_UART("\rMauvaise commande ");
      Send_STR_UART(cmd);
      Send_STR_UART("\rEntrez 'H' pour l'aide\n");
    }
}


/*
 * http://mitchtech.net/msp430-launchpad-pwm/
 */



