/*
 * -----------------------------------------------------
 *  P R O G R A M M A T I O N   S P I   E S I G E L E C
 *
 * Lauchpad v1.5 support
 *
 * M S P 4 3 0 G 2 2 3 1   -   SPI/SLAVE 3 Wires
 *
 * (c)-Yann DUCHEMIN / ESIGELEC - r.III162018 for CCS
 * --------------------------------------------------------------
 * La carte Launchpad est raccordée en SPI via l'USI B0
 *      SCLK : P1.5 / SCLK
 *      SIMO : P1.7 / SDI
 *      MOSI : P1.6 / SDO
 *
 * A la reception du caractère 1 la LED Rouge P1.0 est allumée
 *
 * A la reception du caractère 0 la LED Rouge P1.0 est eteinte
 *
 */
#include <msp430.h> 
#include <intrinsics.h>
#include<stdint.h>
#include<stdbool.h>
#include <string.h>
#include <stdio.h>

#define _UV_SENSOR  BIT4            // A4
#define _CS         BIT4            // chip select for SPI Master->Slave ONLY on 4 wires Mode
#define SCK         BIT5            // Serial Clock
#define DATA_OUT    BIT6            // DATA out
#define DATA_IN     BIT7            // DATA in

volatile unsigned char RXDta = 0x55;
volatile unsigned int Data;

/*
 * Appel aux fonctions
 */
void delay(void);
void init_USCI(void);
void init_ADC( void );
uint16_t ReadADC( uint16_t analog );
uint16_t LowPassReadADC( uint16_t analog );

/*
 * main.c
 */
void main( void )
{
    // Stop watchdog timer to prevent time out reset
    WDTCTL = WDTPW | WDTHOLD;

    if(CALBC1_1MHZ==0xFF || CALDCO_1MHZ==0xFF)
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

    init_USCI();

    //UCB0STAT = 0x23;  // hash, just mean ready; USISRL Vs USIR by ~UCA016B set to 0
    //UCB0MCTL = 0x08;

    if (RXDta == 0x55) //if the input buffer is O for Open (mainly to read the buffer)
    {
        uint16_t indiceUV = 0;
        uint16_t uv_level = 0;
        uv_level = LowPassReadADC(_UV_SENSOR);
        char UV[1];

        if(uv_level<50)
        {
           indiceUV = 0;
        }
        else if(((uint16_t)uv_level >= 50) &&  ((uint16_t)uv_level < 272))
        {
           indiceUV = 1;
        }
        else if(((uint16_t)uv_level >= 272) &&  ((uint16_t)uv_level < 363))
        {
           indiceUV = 2;
        }
        else if(((uint16_t)uv_level >= 363) &&  ((uint16_t)uv_level < 455))
        {
           indiceUV = 3;
        }
        else if(((uint16_t)uv_level >= 455) &&  ((uint16_t)uv_level < 554))
        {
           indiceUV = 4;
        }
        else if(((uint16_t)uv_level >= 554) &&  ((uint16_t)uv_level < 651))
        {
           indiceUV = 5;
        }
        else if(((uint16_t)uv_level >= 651) && ((uint16_t)uv_level < 745))
        {
           indiceUV = 6;
        }
        else if(((uint16_t)uv_level >= 745) &&  ((uint16_t)uv_level < 838))
        {
           indiceUV = 7;
        }
        else if(((uint16_t)uv_level >= 838) &&  ((uint16_t)uv_level < 928))
        {
           indiceUV = 8;
        }
        else if(((uint16_t)uv_level >= 928) &&  ((uint16_t)uv_level < 1027))
        {
           indiceUV = 9;
        }
        else if(((uint16_t)uv_level >= 1027) &&  ((uint16_t)uv_level < 1124))
        {
           indiceUV = 10;
        }
        else //if(uv_level >= 1124)
        {
           indiceUV = 11;
        }

        UV[0] = (char)(indiceUV + 0x30);
        Data = UV[0];

    }

    // Wait for the SPI clock to be idle (low).
    while ((P1IN & BIT5)) {} ;

    while(!(IFG2 & UCB0TXIFG)){};
    UCB0TXBUF = Data;
    //UCB0TXBUF = RXDta;


    P1OUT |= BIT5; //CS unselect

    UCB0TXBUF &= ~UCSWRST;

    __bis_SR_register(LPM4_bits | GIE); // general interrupts enable & Low Power Mode
}

/*
 * Fonctions
 */
void delay(void)
{
    volatile unsigned long i;
    i = 49999;
    do (i--);
    while (i != 0);
}

void init_USCI( void )
{
    /*
    UCB0CTL0 = 0;
    UCB0CTL1 = (0 + UCSWRST*1 );

    IFG2 |= UCB0RXIFG;

    UCB0CTL0 |= ( UCMST | UCMODE_0 | UCSYNC );
    UCB0CTL0 &= ~( UCCKPH | UCCKPL | UCMSB | UC7BIT );
    UCB0CTL1 |= UCSSEL_2;

    UCB0BR0 = 0x0A;     // divide SMCLK by 10
    UCB0BR1 = 0x00;

    // SPI : Fonctions secondaires
    // MISO-1.6 MOSI-1.7 et CLK-1.5
    // Ref. SLAS735G p48,49


    UCB0CTL1 &= ~UCSWRST;                                // activation USCI
    */

    P1SEL  |= ( SCK | DATA_OUT | DATA_IN);
    P1SEL2 |= ( SCK | DATA_OUT | DATA_IN);
    UCB0CTL1 = UCSWRST;
    UCB0CTL0  |= (UCMODE_0 | UCSYNC );
    UCB0CTL0  &= ~(UCMST | UCCKPH | UCCKPL | UCMSB | UC7BIT );
    UCB0CTL1 &= ~UCSWRST;                     // Initialize USCI
    IE2 |= UCA0RXIE;                          // Enable USCI0 RX interrupt
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
uint16_t ReadADC(uint16_t analog)
{
    ADC10CTL1 |= (analog * 0x1000u );
    ADC10CTL0 |= ENC | ADC10SC;
    while( !(ADC10CTL0 & ADC10IFG) || (ADC10CTL1 & ADC10BUSY) );
    ADC10CTL0 &= ~( ENC | ADC10IFG);
    ADC10CTL1 &= ~(analog * 0x1000u);
    return ADC10MEM;
}

/* ----------------------------------------------------------------------------
 * Fonction lecture de la valeur ADC convertie avec filtre passe bas
 * Entrees : numero de voie
 * Sorties: valeur convertie
 */
uint16_t LowPassReadADC(uint16_t analog)
{
    uint16_t lp = 0;
    uint16_t i = 0;
    for( i=63; i > 1; i--)
       lp += ReadADC(analog);
    return(lp>>6);
}
