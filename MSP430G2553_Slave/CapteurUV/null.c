#include <msp430.h> 
#include <string.h>

#define CMDLEN  12   /*longueur maximale de la commande utilisateur*/
#define LF      0x0A
#define CR      0x0D


#define TXLED BIT0
#define RXLED BIT6
#define TXD BIT2
#define RXD BIT1
unsigned int i; //Counter
const char string[] = { "Hello World\r\n" };


void Init_IO(void)
{
    P1DIR |= BIT0 | BIT6;
    P1OUT &= ~(BIT0 | BIT6);
    P1REN |= BIT6;
}

void InitUART(void)
{
    P1SEL |= (BIT1 + BIT2);     /* P1.1 = RXD, P1.2 = TXD */
    P1SEL2 |= (BIT1 + BIT2);    /* P1.1 = RXD, P1.2 = TXD */
    UCA0CTL1 |= UCSSEL_2;       /* SMCLK */
    UCA0BR0 = 104;              /* 1MHz, 9600 */
    UCA0BR1 = 0;                /* 1MHz, 9600 */
    UCA0CTL0 &= ~UCPEN & ~UCPAR & UCMSB;
    UCA0CTL0 &= ~UC7BIT & ~UCSPB & ~UCMODE1;
    UCA0CTL0 &= ~UCMODE0 & ~UCSYNC;
    UCA0CTL1 &= ~UCSWRST;       /* Initialize USCI state machine */
}

void RXdata(unsigned char *c)
{
    while (!(IFG2&UCA0RXIFG));      /* buffer Rx USCI_A0 plein ? */
    *c = UCA0RXBUF;
}

void RXdataVal(unsigned int *val)
{
    while (!(IFG2&UCA0RXIFG));      /* buffer Rx USCI_A0 plein ? */
    *val = UCA0RXBUF;
}

void TXdata(unsigned char c)
{
    while(!(IFG2&UCA0TXIFG));       /* buffer Tx USCI_A0 vide ? */
    UCA0TXBUF = c;
}

void TXdataVal(unsigned int val)
{
    while(!(IFG2&UCA0TXIFG));       /* buffer Tx USCI_A0 vide ? */
    UCA0TXBUF = val;
}

void Send_STR_UART(const char *msg)
{
    int i = 0;
    for(i = 0; msg[i] != 0x00; i++)
    {
        TXdata(msg[i]);
    }
}

void Send_INT_UART(unsigned int *val)
{
        RXdata(&val);
}

void command( char *cmd )
{
  if(strcmp(cmd, "h") == 0)          // aide
  {
    P1OUT |= BIT6;
    Send_STR_UART("\r\nAide : commandes existantes :\n");
    Send_STR_UART("\r\n'ver' : version\n");
    Send_STR_UART("\r\n'l' : allumage led\n");
    Send_STR_UART("\r\n'e' : extinction led\n");
    Send_STR_UART("\r\n'h' : affichage de cette aide\n");
  }
  else if (strcmp(cmd, "ver") == 0)     // version
  {
    Send_STR_UART("\rDebug MSP430 v1.0\n");
  }
  else if(strcmp(cmd, "l") == 0)     // allumage led rouge
  {
    P1OUT |= BIT0;
    Send_STR_UART("\rAllumage led rouge\n");
  }
  else if(strcmp(cmd, "e") == 0)     // extinction led rouge
  {
    P1OUT &= ~BIT0;
    Send_STR_UART("\rExtinction led rouge\n");
  }
  else
  {
    Send_STR_UART("\rMauvaise commande ");
    Send_STR_UART(cmd);
    Send_STR_UART("\rEntrez 'h' pour l'aide\n");
  }
}

void main(void)
{
    char cmd[CMDLEN];       /* tableau de caractere lie a la commande user */
    int nb_car;             /* compteur nombre de caracteres saisis */
    unsigned char c;

    UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt
    __bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ int until Byte RXed*/





    WDTCTL = WDTPW + WDTHOLD;   //Stop WDT

    // clock calibration verification
    if(CALBC1_1MHZ==0xFF || CALDCO_1MHZ==0xFF)
          __low_power_mode_4();
    // factory calibration parameters
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt

    Init_IO();
    InitUART();

    //nb_car = 0;
    Send_STR_UART("MSP430 Ready !\n");
    while(1)
    {

        P1IN |= BIT0;
        int valeurAnalog = P1IN;         // valeur recue par le MSP
        int indiceUV;             // valeur convertie en indice UV de la valeur analogique lue

        if(valeurAnalog < 20)
        {
          indiceUV = 0;
          //char indiceUVStr = indiceUV+'0';
          Send_STR_UART("Indice UV 1: \n");
          while(!(IFG2&UCA0TXIFG));       // buffer Tx USCI_A0 vide ?
          UCA0TXBUF = (char)indiceUV;
          Send_INT_UART(indiceUV);
          //TXdataVal(indiceUV);
          //break;
        }
        else
        {

            indiceUV = 0.05*valeurAnalog-1;
          //char indiceUVStr = (char)indiceUV;
          Send_STR_UART("Indice UV 2: \n");
          while(!(IFG2&UCA0TXIFG));       // buffer Tx USCI_A0 vide ?
          UCA0TXBUF = (char)indiceUV;
          UCA0TXBUF = (char)indiceUV;
          Send_INT_UART(UCA0TXBUF);
          Send_INT_UART(indiceUV);
          //TXdataVal(indiceUV);
          //TXdata((unsigned char)indiceUV);
          break;
        }
        //sleep(2000);

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

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    P1OUT |= TXLED;
    unsigned int indiceUV;
    if(UCA0RXBUF == 'a')
    {
      indiceUV = 0;
      Send_STR_UART("Indice UV 1 :\n");
      Send_INT_UART(indiceUV);
      UCA0TXBUF = indiceUV; // TX next character
    }
    else
    {
      indiceUV = 0.05*TXLED-1;
      Send_STR_UART("Indice UV 2:\n");
      Send_INT_UART(indiceUV);
    }

    //if (i == sizeof string - 1) // TX over?
    UC0IE &= ~UCA0TXIE; // Disable USCI_A0 TX interrupt
    P1OUT &= ~TXLED;

    //int valeurAnalog = UCA0RXBUF;         // valeur recue par le MSP

}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{

     P1OUT |= RXLED;
     if (UCA0RXBUF >= 0) // 'a' received?
     {
        i = 0;
        UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt
        UCA0TXBUF = string[i++];
     }
     P1OUT &= ~RXLED;


    /*int valeurAnalog = UCA0RXBUF;         // valeur recue par le MSP
    int indiceUV;             // valeur convertie en indice UV de la valeur analogique lue
    if(valeurAnalog < 20)
    {
      indiceUV = 0;
      Send_STR_UART("Indice UV 1 :\n");
      Send_STR_UART(indiceUV);
    }
    else
    {
      indiceUV = 0.05*valeurAnalog-1;
      Send_STR_UART("Indice UV 2:\n");
      Send_STR_UART(indiceUV);
    }
    UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt*/

}

/*
 * This sample code is for testing the UV Sensor.
 * Connection:
 *  VCC-5V
 *  GND-GND
 *  OUT-Analog pin 0
 */

/*void setup()
{
    Serial.begin(9600); /*open serial port, set the baud rate to 9600 bps*/
/*}*/

/*void main()
{
    int sensorValue;
    int analogValue = analogRead(0); /*connect UV sensors to Analog 0*/

    /*if(analogValue < 20)
    {
        sensorValue = 0;
    }
    else
    {
        sensorValue = 0.05*analogValue-1;
    }
    Serial.println(sensorValue); /*print the value to serial*/
/*}*/
