/*-------------------------------------------------------------------
                    Declaration des #include
-------------------------------------------------------------------*/
#include <msp430g2553.h>
#include <string.h>
#include <stdio.h>
/*-------------------------------------------------------------------
                    Declaration des #define
-------------------------------------------------------------------*/
#define CMDLEN  12               // longueur maximum de la commande utilisateur
#define LF      0x0A
#define CR      0x0D
#define UCAORXBUF
#define UCAOTXBUF
/*-------------------------------------------------------------------
                    Declaration des fonctions
-------------------------------------------------------------------*/
void Init_IO(void)
{
   P1DIR |= BIT0 | BIT6;  // port 1.0  en sortie
   P1OUT &= ~(BIT0 | BIT6);  // force etat bas P1.0 - LED1
   P1REN |= BIT6;
}

void InitUART(void)
{
    /*P1SEL |= (BIT1 + BIT2);                 // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= (BIT1 + BIT2);                // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                   // SMCLK
    UCA0BR0 = 104;                          // 1MHz, 9600
    UCA0BR1 = 0;                            // 1MHz, 9600
    UCA0CTL0 &= ~UCPEN & ~UCPAR & ~UCMSB;
    UCA0CTL0 &= ~UC7BIT & ~UCSPB & ~UCMODE1;
    UCA0CTL0 &= ~UCMODE0 & ~UCSYNC;
    UCA0CTL1 &= ~UCSWRST;                   // **Initialize USCI state machine**
    */
    BCSCTL1 = CALBC1_1MHZ;                    /* Set DCO*/
    DCOCTL = CALDCO_1MHZ;
    P1SEL = BIT1 + BIT2 ;                     /* P1.1 = RXD, P1.2=TXD*/
    P1SEL2 = BIT1 + BIT2 ;                    /* P1.1 = RXD, P1.2=TXD*/
    UCA0CTL1 |= UCSSEL_2;                     /* SMCLK*/
    UCA0BR0 = 104;                            /* 1MHz 9600*/
    UCA0BR1 = 0;                              /* 1MHz 9600*/
    UCA0MCTL = UCBRS0;                        /* Modulation UCBRSx = 1*/
    UCA0CTL1 &= ~UCSWRST;                     /* **Initialize USCI state machine*/
    IE2 |= UCA0RXIE;                          /* Enable USCI_A0 RX interrupt*/
}

void RXdata(unsigned char *c)
{
    while (!(IFG2&UCA0RXIFG));              // buffer Rx USCI_A0 plein ?
    *c = UCA0RXBUF;
}

void TXdata(unsigned char c)
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

void delay(void)
{
    volatile unsigned int i;
    i = 49999U;
    do(i--);
    while(i != 0);
}

void InitPWM(void)
{
    P1DIR |= BIT6;                          /* P1.6/TA0.1 is used for PWM, thus also an output -> servo 1 */
    P1DIR &= ~ BIT1;
    P1OUT = 0;                              /* Clear all outputs P1 */

    P1SEL |= BIT6;                          /* P1.6 select TA0.1 option */

    /* if SMCLK is about 1MHz (or 1000000Hz), */
    /* and 1000ms are the equivalent of 1 Hz, */
    /* then, by setting CCR0 to 20000 (1000000 / 1000 * 20) */
    /* we get a period of 20ms */
    TA0CCR0 = 20000-1;                           /* PWM Period TA0.1 */

    /* setting 1500 is 1.5ms is 0deg. servo pos */
    TA0CCR1 = 1500;                            /* CCR1 PWM duty cycle */
    TA0CCTL1 = OUTMOD_7;                       /* CCR1 reset/set */
    TA0CTL   = TASSEL_2 + MC_1;                /* SMCLK, up mode */
}
void command(char *cmd)
{
  if(strcmp(cmd, "h") == 0)          // aide
  {
    P1OUT |= BIT6;
    // PWM : Go to 0°
    TA0CCR1 = 1000;
    delay();
    Send_STR_UART("\r\nAide : commandes existantes :\n");
    Send_STR_UART("\r\n'ver' : version\n");
    Send_STR_UART("\r\n'l' : allumage led\n");
    Send_STR_UART("\r\n'e' : extinction led\n");
    Send_STR_UART("\r\n'h' : affichage de cette aide\n");
    Send_STR_UART("\r\n'o' : ouverture serre\n");
    Send_STR_UART("\r\n'f' : fermeture serre\n");
  }
  else if (strcmp(cmd, "ver") == 0)     // version
  {
    Send_STR_UART("\r\nDebug MSP430 v1.0\n");
  }
  else if(strcmp(cmd, "l") == 0)     // allumage led verte
  {
    P1OUT |= BIT0;
    Send_STR_UART("\r\nAllumage led verte\n");
  }
  else if(strcmp(cmd, "e") == 0)     // extinction led verte
  {
    P1OUT &= ~BIT0;
    Send_STR_UART("\r\nExtinction led verte\n");
  }
  else if(strcmp(cmd, "o") == 0)     // ouverture de la serre
    {
      // Go to 180°
      P1OUT |= BIT0;
      TA0CCR1 = 2000;
      delay();
      Send_STR_UART("\r\nOuverture serre\n");
    }
  else if(strcmp(cmd, "f") == 0)     // fermeture serre
    {
      P1OUT &= ~BIT0;
      // PWM : Go to 0°
      TA0CCR1 = 1000;
      delay();
      Send_STR_UART("\r\nFermeture serre\n");
    }
  else
  {
    Send_STR_UART("\r\nMauvaise commande ");
    Send_STR_UART(cmd);
    Send_STR_UART("\r\nEntrez 'h' pour l'aide\n");
  }
}

/*-------------------------------------------------------------------
                   Programme principal
-------------------------------------------------------------------*/
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
    P1IE |= BIT6;
    P1IES |= BIT6;
    P1IFG &= ~(BIT6);

    Init_IO();
    InitUART();
    InitPWM();
    nb_car = 0;
    Send_STR_UART("MSP430 Ready !");

    while(1)
    {
        if( nb_car<(CMDLEN-1) )
        {
            __enable_interrupt(); // autorisation generale des interruptions
            _BIS_SR(GIE);
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

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    int i =0;
    int j=0;
    int menu;

    Send_STR_UART("=== Menu ===\n\n");
    Send_STR_UART("h. Aide\n");
    Send_STR_UART("ver. Version \n");
    Send_STR_UART("l. Allumer led verte\n");
    Send_STR_UART("e. Eteindre led verte\n");
    Send_STR_UART("o. Ouverture serre\n");
    Send_STR_UART("f. Fermeture serre\n");
    Send_STR_UART("\nVotre choix ? ");

    while((IFG2&UCA0TXIFG)==0)
    {
    }
    switch(UCA0RXBUF)
    {
    case 'h':
        for(i=0;i<sizeof(menu);i++)
        {
            P1OUT |= BIT6;
            UCA0TXBUF=P1OUT;
            // PWM : Go to 0°
            TA0CCR1 = 1000;
            delay();
            Send_STR_UART("\r\nAide : commandes existantes :\n");
            Send_STR_UART("\r\n'ver' : version\n");
            Send_STR_UART("\r\n'l' : allumage led\n");
            Send_STR_UART("\r\n'e' : extinction led\n");
            Send_STR_UART("\r\n'h' : affichage de cette aide\n");
            Send_STR_UART("\r\n'o' : ouverture serre\n");
            Send_STR_UART("\r\n'f' : fermeture serre\n");
            for(j=0;j<100;j++){};
        }
        break;
    case 'v':
        Send_STR_UART("\r\nDebug MSP430 v1.0\n");
        break;
    case 'l':
        P1OUT |= BIT0;
        Send_STR_UART("\r\nAllumage led verte\n");
        break;
    case 'e':
        P1OUT &= ~BIT0;
        Send_STR_UART("\r\nExtinction led verte\n");
        break;
    case 'o':
        P1OUT |= BIT0;
         //Go to 180°
        TA0CCR1 = 2000;
        delay();
        Send_STR_UART("\r\nOuverture serre\n");
        break;
    case 'f':
        P1OUT &= ~BIT0;
        // PWM : Go to 0°
        TA0CCR1 = 1000;
        delay();
        Send_STR_UART("\r\nFermeture serre\n");
        break;
    default:
        Send_STR_UART("\r\nMauvaise commande ");
        //Send_STR_UART(cmd);
        Send_STR_UART("\r\nEntrez 'h' pour l'aide\n");
        break;
    }
}
