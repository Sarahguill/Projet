/*
 * uart.c
 *
 *  Created on: 20 mars 2021
 *      Author: Capucine Bourgade
 */

#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define     CMDLEN          12               // longueur maximum de la commande utilisateur
#define     LF              0x0A
#define     CR              0x0D


/***********************************
 * Appel aux fonctions
 ***********************************/
void InitUART(void);
void RXdata(unsigned char *c);
void TXdata(unsigned char c);
void Send_STR_UART(const char *msg);
void command( char *cmd );


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

void command( char *cmd )
{

    unsigned int i;

    if(strcmp(cmd, "H") == 0)          // aide
      {
        __delay_cycles(100000);

        //P1OUT |= BIT6;
        Send_STR_UART("\r\nAide : Commandes existantes :\n");
        Send_STR_UART("\r\n'H' : Afficher l'aide\n");
        Send_STR_UART("\r\n'U' : Afficher l'indice UV\n");
      }
    else if (strcmp(cmd, "U") == 0)     // ouverture
    {
        __delay_cycles(100000);

        Send_STR_UART("\rIndice UV : \n");
    }
    else
    {
      Send_STR_UART("\rMauvaise commande ");
      Send_STR_UART(cmd);
      Send_STR_UART("\rEntrez 'H' pour l'aide\n");
    }
}
