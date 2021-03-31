#include <msp430.h> 
#include <string.h>


/*
 * Prototypes
 */
void init_BOARD( void );
void init_UART( void );
void RXdata(unsigned char *c);
void TXdata(unsigned char c);
void Send_STR_UART(const char *msg);
void init_USCI( void );
void interpreteur( void );
void envoi_msg_UART(unsigned char * );
void Send_char_SPI( unsigned char );
void InitPWM(void);
void delay(void);

/*
 * Definitions
 */
#define RELEASE "\r\t\tSPI-rIII162018"
#define PROMPT  "\r\nmaster>"
#define CMDLEN  10

#define TRUE    1
#define FALSE   0

#define LF      0x0A            // line feed or \n
#define CR      0x0D            // carriage return or \r
#define BSPC    0x08            // back space
#define DEL     0x7F            // SUPRESS
#define ESC     0x1B            // escape

#define _CS         BIT4            // chip select for SPI Master->Slave ONLY on 4 wires Mode
#define SCK         BIT5            // Serial Clock
#define DATA_OUT    BIT6            // DATA out
#define DATA_IN     BIT7            // DATA in


/*
 * Variables globales
 */
// static const char spi_in = 0x37;
unsigned char cmd[CMDLEN];      // tableau de caracteres lie a la commande user
unsigned char car = 0x30;       // 0
unsigned int  nb_car = 0;
unsigned char intcmd = FALSE;   // call interpreteur()
unsigned char *dataReceived;

/* ----------------------------------------------------------------------------
 * Fonction d'interpretation des commandes utilisateur
 * Entrees: -
 * Sorties:  -
 */
void interpreteur( void )
{
    if(strcmp((const char *)cmd, "H") == 0)          //----------------------------------- help
    {
        envoi_msg_UART("\r\nCommandes :");
        envoi_msg_UART("\r\n'ver' : version");
        envoi_msg_UART("\r\n'O' : Ouverture Serre");
        envoi_msg_UART("\r\n'F' : Fermeture Serre");
        envoi_msg_UART("\r\n'U': Recuperation valeur UV");
        envoi_msg_UART("\r\n'H' : help\r\n");
    }
    else if (strcmp((const char *)cmd, "O") == 0)
    {
        envoi_msg_UART("\r\n");
        envoi_msg_UART((unsigned char *)cmd);
        envoi_msg_UART("-> Serre ouverte");
        envoi_msg_UART("\r\n");

        // Go to 180°
        TA1CCR1 = 2500;
        delay();
    }
    else if (strcmp((const char *)cmd, "F") == 0)
    {
        envoi_msg_UART("\r\n");
        envoi_msg_UART((unsigned char *)cmd);
        envoi_msg_UART("-> Serre fermee");
        envoi_msg_UART("\r\n");

        // PWM : Go to 0°
        TA1CCR1 = 500;
        delay();
    }
    else if (strcmp((const char *)cmd, "ver") == 0)
    {
        envoi_msg_UART("\r\n");
        envoi_msg_UART(RELEASE);
        envoi_msg_UART("\r\n");
    }
    else if (strcmp((const char *)cmd, "U") == 0)
    {
        envoi_msg_UART("\r\n");
        envoi_msg_UART((unsigned char *)cmd);
        envoi_msg_UART("->");
        // IFG2 = 1;
        Send_char_SPI(0x55);                    // Send 'U' over SPI to Slave
        Send_STR_UART("\r\nIndice UV : ");
        while(!(IFG2 & UCB0RXIFG)){};
        dataReceived = (unsigned char*)UCB0RXBUF;
        Send_STR_UART((const char*)dataReceived);
        envoi_msg_UART("\r\n");
        // IFG2 = 0;
    }
    else                          //---------------------------- default choice
    {
        envoi_msg_UART("\r\n ?");
        envoi_msg_UART((unsigned char *)cmd);
    }
    envoi_msg_UART(PROMPT);        //---------------------------- command prompt
}

/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de la carte TI LauchPAD
 * Entrees: -
 * Sorties:  -
 */
void init_BOARD( void )
{
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW | WDTHOLD;

    if( (CALBC1_1MHZ==0xFF) || (CALDCO_1MHZ==0xFF) )
    {
        __bis_SR_register(LPM4_bits);
    }
    else
    {
        // Factory Set.
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = (0 | CALDCO_1MHZ);
    }

    //--------------- Secure mode
    P1SEL  = 0x00;        // GPIO
    P1SEL2 = 0x00;        // GPIO
    P2SEL  = 0x00;        // GPIO
    P2SEL2 = 0x00;        // GPIO
    P1DIR = 0x00;         // IN
    P2DIR = 0x00;         // IN


    P1SEL  |= ( SCK | DATA_OUT | DATA_IN);
    P1SEL2 |= ( SCK | DATA_OUT | DATA_IN);

    P1DIR |= SCK ;  // LED: output
    P1OUT &= ~SCK ;
}

/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de l'UART
 * Entree : -
 * Sorties: -
 */
void init_UART( void )
{
    P1SEL  |= (BIT1 | BIT2);                    // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= (BIT1 | BIT2);                    // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSWRST;                        // SOFTWARE RESET
    UCA0CTL1 |= UCSSEL_2;                       // SMCLK (2 - 3)
    UCA0BR0 = 104;                             // 104 1MHz, OSC16, 9600 (8Mhz : 52) : 8 115k - 226/12Mhz
    UCA0BR1 = 0;                                // 1MHz, OSC16, 9600 - 4/12Mhz
    UCA0MCTL = 10;
    UCA0CTL0 &= ~(UCPEN  | UCMSB | UCDORM);
    UCA0CTL0 &= ~(UC7BIT | UCSPB  | UCMODE_3 | UCSYNC); // dta:8 stop:1 usci_mode3uartmode
    UCA0CTL1 &= ~UCSWRST;                       // **Initialize USCI state machine**
    /* Enable USCI_A0 RX interrupt */
    IE2 |= UCA0RXIE;
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

/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de l'USCI POUR SPI SUR UCB0
 * Entree : -
 * Sorties: -
 */
void init_USCI( void )
{
    // Waste Time, waiting Slave SYNC
    __delay_cycles(250);

    // SOFTWARE RESET - mode configuration
    UCB0CTL0 = 0;
    UCB0CTL1 = (0 + UCSWRST*1 );

    // clearing IFg /16.4.9/p447/SLAU144j
    // set by setting UCSWRST just before
    //IFG2 &= ~(UCB0TXIFG | UCB0RXIFG);
    IFG2 |= UCB0TXIFG;

    // Configuration SPI (voir slau144 p.445)
    // UCCKPH = 0 -> Data changed on leading clock edges and sampled on trailing edges.
    // UCCKPL = 0 -> Clock inactive state is low.
    //   SPI Mode 0 :  UCCKPH * 1 | UCCKPL * 0
    //   SPI Mode 1 :  UCCKPH * 0 | UCCKPL * 0  <--
    //   SPI Mode 2 :  UCCKPH * 1 | UCCKPL * 1
    //   SPI Mode 3 :  UCCKPH * 0 | UCCKPL * 1
    // UCMSB  = 1 -> MSB premier
    // UC7BIT = 0 -> 8 bits, 1 -> 7 bits
    // UCMST  = 0 -> CLK by Master, 1 -> CLK by USCI bit CLK / p441/16.3.6
    // UCMODE_x  x=0 -> 3-pin SPI,
    //           x=1 -> 4-pin SPI UC0STE active high,
    //           x=2 -> 4-pin SPI UC0STE active low,
    //           x=3 -> i²c.
    // UCSYNC = 1 -> Mode synchrone (SPI)
    UCB0CTL0 |= ( UCMST | UCMODE_0 | UCSYNC );
    UCB0CTL0 &= ~( UCCKPH | UCCKPL | UCMSB | UC7BIT );
    UCB0CTL1 |= UCSSEL_2;

    UCB0BR0 = 0x20;     // divide SMCLK by 10
    UCB0BR1 = 0x00;

    // SPI : Fonctions secondaires
    // MISO-1.6 MOSI-1.7 et CLK-1.5
    // Ref. SLAS735G p48,49
    P1SEL  |= ( SCK | DATA_OUT | DATA_IN);
    P1SEL2 |= ( SCK | DATA_OUT | DATA_IN);

    UCB0CTL1 &= ~UCSWRST;                                // activation USCI
}

/* ----------------------------------------------------------------------------
 * Fonction d'emission d'une chaine de caracteres
 * Entree : pointeur sur chaine de caracteres
 * Sorties:  -
 */
void envoi_msg_UART(unsigned char *msg)
{
    unsigned int i = 0;
    for(i=0 ; msg[i] != 0x00 ; i++)
    {
        while(!(IFG2 & UCA0TXIFG));    //attente de fin du dernier envoi (UCA0TXIFG à 1 quand UCA0TXBUF vide)
        UCA0TXBUF=msg[i];
    }
}

/* ----------------------------------------------------------------------------
 * Fonction d'envoie d'un caractère sur USCI en SPI 3 fils MASTER Mode
 * Entree : Caractère à envoyer
 * Sorties: /
 */
void Send_char_SPI(unsigned char carac)
{
    while ((UCB0STAT & UCBUSY)){};   // attend que USCI_SPI soit dispo.
    while(!(IFG2 & UCA0TXIFG)){}; // p442
    UCB0TXBUF = carac;              // Put character in transmit buffer
    envoi_msg_UART((unsigned char *)cmd);   // slave echo
}

void InitPWM(void)
{
    P2DIR |= BIT2;                          // P2.2/TA1.1 is used for PWM, thus also an output -> servo 2
    P2OUT = 0;                              // Clear all outputs P2
    P2SEL |= BIT2;                          // P2.2 select TA1.1 option

    // if SMCLK is about 1MHz (or 1000000Hz),
    // and 1000ms are the equivalent of 1 Hz,
    // then, by setting CCR0 to 20000 (1000000 / 1000 * 20)
    // we get a period of 20ms
    TA1CCR0 = 20000-1;                           // PWM Period TA1.1

    // setting 1500 is 1.5ms is 0deg. servo pos
    TA1CCR1 = 1500;                            // CCR1 PWM duty cycle
    TA1CCTL1 = OUTMOD_7;                       // CCR1 reset/set
    TA1CTL   = TASSEL_2 + MC_1;                // SMCLK, up mode
}

void delay(void)
{
    volatile unsigned long i;
    i = 49999;
    do (i--);
    while (i != 0);
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * main.c
 */
void main( void )
{

    init_BOARD();
    init_UART();
    init_USCI();
    InitPWM();

    envoi_msg_UART("\rReady !\r\n"); // user prompt
    envoi_msg_UART(PROMPT);        //---------------------------- command prompt

 while(1)
    {
     __enable_interrupt();          // autorisation generale des interruptions
     _BIS_SR(GIE);
     if(intcmd)
        {

            while ((UCB0STAT & UCBUSY));    // attend que USCI_SPI soit dispo.
            intcmd = FALSE;                 // acquitte la commande en cours
            P1OUT |= BIT5;                  // Unselect Device
        }
        else
        {
            __bis_SR_register(LPM4_bits | GIE); // general interrupts enable & Low Power Mode
        }
    }

}

//------------------------------------------------------------------ End ISR
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR()
{
    //---------------- UART
    if (IFG2 & UCA0RXIFG)
    {
        while(!(IFG2 & UCA0RXIFG));
        cmd[nb_car]=UCA0RXBUF;         // lecture caractère reçu

        while(!(IFG2 & UCA0TXIFG));    // attente de fin du dernier envoi (UCA0TXIFG à 1 quand UCA0TXBUF vide) / echo
        UCA0TXBUF = cmd[nb_car];

        if( cmd[nb_car] == ESC)
        {
            nb_car = 0;
            cmd[1] = 0x00;
            cmd[0] = CR;
        }

        if( (cmd[nb_car] == CR) || (cmd[nb_car] == LF))
        {
            cmd[nb_car] = 0x00;
            intcmd = TRUE;
            nb_car = 0;
            __bic_SR_register_on_exit(LPM4_bits);   // OP mode !
        }
        else if( (nb_car < CMDLEN) && !((cmd[nb_car] == BSPC) || (cmd[nb_car] == DEL)) )
        {
            nb_car++;
        }
        else
        {
            cmd[nb_car] = 0x00;
            nb_car--;
        }
    }
    else if (IFG2 & UCB0TXBUF) // SPI
    {
        if (IFG2 & UCB0TXBUF){
            while( (UCB0STAT & UCBUSY) && !(UCB0STAT & UCOE) );
            while(!(IFG2 & UCB0RXIFG));
            cmd[0] = UCB0TXBUF;
            cmd[1] = 0x00;
        }
    }

    //Menu Interpreteur de commandes
    int i =0;
    int j=0;
    int menu;

    while((IFG2&UCA0TXIFG)==0)
    {
    }
    switch(UCA0RXBUF)
    {
    case 'H':
        for(i=0;i<sizeof(menu);i++)
        {
            P1OUT |= BIT6;
            UCA0TXBUF=P1OUT;
            // PWM : Go to 0°
            TA0CCR1 = 1000;
            delay();
            envoi_msg_UART("\r\nAide : commandes existantes :\n");
            envoi_msg_UART("\r\n'ver' : version\n");
            envoi_msg_UART("\r\n'H' : affichage de cette aide\n");
            envoi_msg_UART("\r\n'O' : ouverture serre\n");
            envoi_msg_UART("\r\n'F' : fermeture serre\n");
            envoi_msg_UART("\r\n'U' : Mesure UV\n");
            for(j=0;j<100;j++){};
        }
        break;

    case 'v':
        envoi_msg_UART("\r\nDebug MSP430 v1.0\n");
        break;

    case 'O':
        P1OUT |= BIT0;
         //Go to 180°
        TA1CCR1 = 2500;
        delay();
        envoi_msg_UART("\r\nOuverture serre\n");
        break;

    case 'F':
        P1OUT &= ~BIT0;
        // PWM : Go to 0°
        TA1CCR1 = 500;
        delay();
        envoi_msg_UART("\r\nFermeture serre\n");
        break;

    case 'U':
        envoi_msg_UART("\r\n");
        envoi_msg_UART((unsigned char *)cmd);
        envoi_msg_UART("->");
        // IFG2 = 1;
        Send_char_SPI(0x55);                    // Send 'U' over SPI to Slave
        envoi_msg_UART("\r\nIndice UV : ");
        while(!(IFG2 & UCB0RXIFG)){};
        dataReceived = (unsigned char*)UCB0RXBUF;
        envoi_msg_UART((const char*)dataReceived);
        envoi_msg_UART("\r\n");
        // IFG2 = 0;
        break;

    default:
        envoi_msg_UART("\r\nMauvaise commande ");
        //Send_STR_UART(cmd);
        envoi_msg_UART("\r\nEntrez 'h' pour l'aide\n");
        break;
    }

}


