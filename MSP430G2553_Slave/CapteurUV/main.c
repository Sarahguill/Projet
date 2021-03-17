/*
 * main.c
 *
 *  Created on: 17 mars 2021
 *      Author: Capucine Bourgade
 */




//******************************************************************************
//   MSP430G2xx3 UV sensor SKU (SEN0162)
//
//   Description: read sensor from analog input and return IUV index
//   Built with CCS V10.2.0x - ESIGELEC March 2021
//******************************************************************************
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

/*
 * Program def.
 */
#define     _UV_SENSOR      BIT4        // A4
#define     _BUZZER_PH      BIT2        // TA1.1, Phase
#define     _BUZZER_APH     BIT3        // TA1.0, Anti-Phase

#define     IDX_0           50
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
#define     IDX_11          1000

/*
 * Program const.
 */


/*
 * Prototypes
 */
voidinitClockTo1MHz( void );
voidinitGPIO( void );
voidinit_ADC( void );
uint16_tReadADC( uint16_t );
uint16_tLowPassReadADC( uint16_t );


/*
 * Clock init.
 */
voidinitClockTo1MHz( void )
{
    if( (CALBC1_1MHZ == 0xFF) || (CALDCO_1MHZ == 0xFF) )
    {
        __bis_SR_register(LPM4_bits);   // trap
    }
    else
    {
        // Factory Set.
        DCOCTL  = 0;
        BCSCTL1 = (0 | CALBC1_1MHZ);
        DCOCTL  = (0 | CALDCO_1MHZ);
    }
}


/*
 * GPIO general init.
 */
voidinitGPIO( void )
{
    //--- Secure mode
    P1SEL  = 0x00;        // GPIO
    P1SEL2 = 0x00;
    P2SEL  = 0x00;        // GPIO
    P2SEL2 = 0x00;
    P3SEL  = 0x00;        // GPIO
    P3SEL2 = 0x00;
    P1DIR  = 0x00;        // IN
    P2DIR  = 0x00;        // IN
    P3DIR  = 0x00;        // IN
    P3REN  = 0xFF;
    //---
}

/* ----------------------------------------------------------------------------
 * Fonction d'initialisation de l'ADC
 * Entrees : -
 * Sorties: -
 */
voidinit_ADC( void )
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
uint16_tReadADC(uint16_t analog)
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
uint16_tLowPassReadADC(uint16_t analog)
{
    uint16_t lp = 0;
    uint16_t i = 0;
    for( i=63; i > 1; i--)
        lp += ReadADC(analog);
    return(lp>>6);
}

/*              °
 *    # #   ##  # #  #    ###
 *   # # # #  # # ## #   #
 *   #   # #### # # ##   #
 *   #   # #  # # #  # o  ###
 */
voidmain(void)
{
    WDTCTL = WDTPW | WDTHOLD;// stop watchdog timer

    uint16_t uv_level = 0;

    initClockTo1MHz();
    initGPIO();
    init_ADC();

    do
    {
        uv_level = LowPassReadADC(_UV_SENSOR);
        __delay_cycles(500);
    }while(1);
}
