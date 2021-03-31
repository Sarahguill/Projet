/*
 * main.c
 *
 *  Created on: 17 mars 2021
 *      Author: Capucine Bourgade
 */




/******************************************************************************
   MSP430G2xx3 UV sensor SKU (SEN0162)

  Description: read sensor from analog input and return IUV index
  Built with CCS V10.2.0x - ESIGELEC March 2021
******************************************************************************
*/
/*                       -------------
 *                      / MSP430G2553 \
 *                 |  1 |Vcc      Vss | 20 |
 *           P1.0  |  2 |             | 19 | P2.6
 *           P1.1  |  3 |             | 18 | P2.7
 *           P1.2  |  4 |             | 17 | Tst
 *           P1.3  |  5 |             | 16 | !RST
 *    [UVBs>-P1.4  |  6 |             | 15 | P1.7
 *           P1.5  |  7 |             | 14 | P1.6
 *           P2.0  |  8 |             | 13 | P2.5
 *           P2.1  |  9 |             | 12 | P2.4
 *    [Buz+>-P2.2  | 10 |             | 11 | P2.3-<Buz-]
 *                      \_____________/
 * ---------------------------------------------------------------------------
 */
#include<msp430.h>
#include<stdint.h>
#include<stdbool.h>
#include <string.h>
/* #include <stdio.h> */ 

/*
Program def.
 */
#define     _UV_SENSOR      BIT4        /* A4 */
/*#define     _BUZZER_PH      BIT2 */       /* TA1.1, Phase */
/*#define     _BUZZER_APH     BIT3  */      /* TA1.0, Anti-Phase */
#define     CMDLEN          12U               /* longueur maximum de la commande utilisateur */
/* #define     LF              0x0A */
#define     CR              0x0DU

/*#define     IDX_0           50
#define     IDX_1           150
#define     IDX_2           186
#define     IDX_3           279
#define     IDX_4           372
#define     IDX_5           465
#define     IDX_6           559
#define     IDX_7           652
#define     IDX_8           745
#define     IDX_9           838
#define     IDX_10          931
#define     IDX_11          1000*/

/*
 Program const.
 */


/***********************************
 * Appel aux fonctions/Prototypes
 ***********************************/
void InitUART(void);
void RXdata(UCHAR *c);
void TXdata(UCHAR c);
void Send_STR_UART(UCHAR *msg);
void command( UCHAR *cmd );
void initClockTo1MHz( void );
void initGPIO( void );
void init_ADC( void );
uint16_t ReadADC( UINT16 analog );
uint16_t LowPassReadADC( UINT16 analog );


/***********************************
 * Les fonctions
 ***********************************/

void InitUART(void)
{
    P1SEL |= (BIT1 + BIT2);                 /* P1.1 = RXD, P1.2=TXD */
    P1SEL2 |= (BIT1 + BIT2);                /* P1.1 = RXD, P1.2=TXD */
    UCA0CTL1 |= UCSSEL_2;                   /* SMCLK */
    UCA0BR0 = 104;                          /* 1MHz, 9600 */
    UCA0BR1 = 0;                            /* 1MHz, 9600 */
    UCA0CTL0 &= ~UCPEN & ~UCPAR & ~UCMSB;
    UCA0CTL0 &= ~UC7BIT & ~UCSPB & ~UCMODE1;
    UCA0CTL0 &= ~UCMODE0 & ~UCSYNC;
    UCA0CTL1 &= ~UCSWRST;                   /* **Initialize USCI state machine** */
}

void RXdata(UCHAR *c)
{
    while (!(IFG2&UCA0RXIFG)){};              /* buffer Rx USCI_A0 plein ? */
    *c = UCA0RXBUF;
}

void TXdata( UCHAR c )
{
    while (!(IFG2&UCA0TXIFG)){};              /* buffer Tx USCI_A0 vide ? */
    UCA0TXBUF = c;
}

void Send_STR_UART(UCHAR *msg)
{
    UINT16 i = 0U;
    for(i=0U ; msg[i] != 0x00U ; i++)
    {
        TXdata(msg[i]);
    }
}

void command( UCHAR *cmd )
{

    UINT16 indiceUV = 0;
    UINT16  uv_level = 0;
    uv_level = LowPassReadADC(_UV_SENSOR);
    UCHAR UV[1];

    if(strcmp(cmd, "H") == 0)          /* aide */
      {
        __delay_cycles(100000){};

        P1OUT |= BIT6; 
        Send_STR_UART("\r\nAide : Commandes existantes :\n");
        Send_STR_UART("\r\n'H' : Afficher l'aide\n");
        Send_STR_UART("\r\n'U' : Afficher l'indice UV\n");
      }
    else if (strcmp(cmd, "U") == 0)     /* ouverture */
    {
        __delay_cycles(100000){};
        if(uv_level<50U)
        {
           indiceUV = 0U;
        }
        else if(((UINT16)uv_level >= 50U) &&  ((UINT16)uv_level < 272U))
        {
           indiceUV = 1U;
        }
        else if(((UINT16)uv_level >= 272U) &&  ((UINT16)uv_level < 363U))
        {
           indiceUV = 2U;
        }
        else if(((UINT16)uv_level >= 363U) &&  ((UINT16)uv_level < 455U))
        {
           indiceUV = 3U;
        }
        else if(((UINT16)uv_level >= 455U) &&  ((UINT16)uv_level < 554U))
        {
           indiceUV = 4U;
        }
        else if(((UINT16)uv_level >= 554U) &&  ((UINT16)uv_level < 651U))
        {
           indiceUV = 5U;
        }
        else if(((UINT16)uv_level >= 651U) && ((UINT16)uv_level < 745U))
        {
           indiceUV = 6U;
        }
        else if(((UINT16)uv_level >= 745U) &&  ((UINT16)uv_level < 838U))
        {
           indiceUV = 7U;
        }
        else if(((UINT16)uv_level >= 838U) &&  ((UINT16)uv_level < 928U))
        {
           indiceUV = 8U;
        }
        else if(((UINT16)uv_level >= 928U) &&  ((UINT16)uv_level < 1027U))
        {
           indiceUV = 9U;
        }
        else if(((UINT16)uv_level >= 1027U) &&  ((UINT16)uv_level < 1124U))
        {
           indiceUV = 10U;
        }
        else /*if(uv_level >= 1124U) */
        {
           indiceUV = 11U;
        }
        UV[0] = ((char)indiceUV + 0x30U);
        Send_STR_UART("\rIndice UV : ");
        Send_STR_UART((const char*)&UV[0]);
        Send_STR_UART("\n");
    }
    else
    {
      Send_STR_UART("\rMauvaise commande ");
      Send_STR_UART(cmd);
      Send_STR_UART("\rEntrez 'H' pour l'aide\n");
    }
}

/*
 Clock init.
 */
void initClockTo1MHz( void )
{
    if((CALBC1_1MHZ == 0xFF) || (CALDCO_1MHZ == 0xFF))
    {
        __bis_SR_register(LPM4_bits){};   /* trap */
    }
    else
    {
        /* Factory Set. */
        DCOCTL  = 0;
        BCSCTL1 = (0 | CALBC1_1MHZ);
        DCOCTL  = (0 | CALDCO_1MHZ);
    }
}


/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de l'ADC
 * Entrees : -
 * Sorties: -
 */
void init_ADC( void )
{
    ADC10CTL0 = 0x00;
    ADC10CTL1 = 0x00;
    ADC10CTL0 |= ( SREF_0 | ADC10SHT_2 | REFON | ADC10ON );
    ADC10CTL0 &= ~( ADC10SR | REF2_5V );
    ADC10CTL1 |= ( ADC10DIV_0 | ADC10SSEL_3 | SHS_0 | CONSEQ_0 );
}

/* ----------------------------------------------------------------------------
 * Fonction lecture de la valeur ADC convertie
 * Entrees : numero de voie
 * Sorties: valeur convertie
 */
UINT16 ReadADC(UINT16 analog)
{
    ADC10CTL1 |= (analog * 0x1000u );
    ADC10CTL0 |= ENC | ADC10SC;
    while( !(ADC10CTL0 & ADC10IFG) || (ADC10CTL1 & ADC10BUSY) ){};
    ADC10CTL0 &= ~( ENC | ADC10IFG);
    ADC10CTL1 &= ~(analog * 0x1000u);
    return ADC10MEM;
}

/* ----------------------------------------------------------------------------
 * Fonction lecture de la valeur ADC convertie avec filtre passe bas
 * Entrees : numero de voie
 * Sorties: valeur convertie
 */
UINT16 LowPassReadADC(UINT16 analog)
{
    UINT16 lp = 0U;
    UINT16 i = 0U;
    for( i=63U; i > 1U; i--){
       lp += ReadADC(analog)};
    return(lp>>6);
}


/***********************************
 * Le main
 ***********************************/
void int main(void)
{

    UCHAR c;
    UCHAR  cmd[CMDLEN];      /* tableau de caractere lie a la commande user */
    UINT16   nb_car;           /* compteur nombre carateres saisis */

    WDTCTL = WDTPW + WDTHOLD;   /* Stop WDT */
    /* clock calibration verification */
    if((CALBC1_1MHZ==0xFF) || (CALDCO_1MHZ==0xFF)){
      __low_power_mode_4()};
    /* factory calibration parameters */
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    InitUART();
    initClockTo1MHz(){};
    initGPIO(){}; 
    init_ADC(){};

    P1SEL &= ~(0x10);
    P1SEL2 &= ~(0x10);
    P1DIR &= ~(0x10);

    nb_car = 0U;
    Send_STR_UART("MSP430 Ready !\n");
    while(1)
    {
        if( nb_car<(CMDLEN-1) )
        {
            RXdata(&c);
            if( (cmd[nb_car]==c) != CR )
            {
                TXdata(c);
                nb_car++;
            }
            else
            {
              cmd[nb_car]=0x00U;
              command(cmd);
              nb_car=0U;
            }
        }
    }
}
