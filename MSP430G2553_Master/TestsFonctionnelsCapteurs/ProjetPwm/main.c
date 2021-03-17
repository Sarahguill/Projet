/*-------------------------------------------------------------------
                    Declaration des #include
-------------------------------------------------------------------*/
#include <msp430.h> 
#include <stdio.h>
#include <time.h>
/*
 * #include <ADC.c>
 * #include <ADC.h>
 * #include <Afficheur.c>
 * #include <Afficheur.h>
*/

/*-------------------------------------------------------------------
                    Declaration des #define
-------------------------------------------------------------------*/

#define     LED_VERTE             BIT6 // LED2 0x40
#define     LED_ROUGE             BIT0 // LED1 0x01
#define     BUTTON                BIT3

/*-------------------------------------------------------------------
                    Declaration des variables
-------------------------------------------------------------------*/
int tempo = 0;
int rapportCyclique = 20;
long compteur=0;
const long baseTemps = 16384;  // valeur pour une période de 0,5 seconde

volatile unsigned int cpt = 0;
volatile unsigned int Result = 0;

/*-------------------------------------------------------------------
                     Programme principal
-------------------------------------------------------------------*/

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;
    BCSCTL1= CALBC1_1MHZ;                   //frequence d’horloge 1MHz
    DCOCTL= CALDCO_1MHZ;

    P2DIR |= BIT2;                          // P2.2 en sortie
    P2SEL |= BIT2;                          // selection fonction TA1.1
    P2SEL2 &= ~BIT2;                        // selection fonction TA1.1
    P2OUT |= BIT2;

    TA0CTL = TASSEL_2 | MC_1 |ID_0;               // source SMCLK pour TimerA (no 2), mode comptage Up
    TA0CCTL1 |= OUTMOD_7;                   // activation mode de sortie n°7
    TA0CCR0 = 2000;                         // determine la periode du signal
    TA0CCR1 = 500;                          // determine le rapport cyclique du signal
    while(1)
    {
        /*if ((P1IN & BIT3) == 1)
        {
            TA1CCR1 = 0;

            P2OUT |= BIT3; // P2.3 = 1,P2.4 = 0
            P2OUT |= BIT6; // P2.6 = 1 ,3&4_EN = 1,Motor is started
            __delay_cycles(10);     // Rotate motor for sometime

        }
        else
        {
            TA1CCR1 = 500;
        }*/
    };

}

/*-------------------------------------------------------------------
                        Fonctions
-------------------------------------------------------------------*/

void attente_timer()
{
	while(!(TA0CTL & TAIFG)){};           //Attente flag TAIFG a 1
	TA0CTL &= ~TAIFG;                   //RAZ TAIFG
}



/*-------------------------------------------------------------------
                     Interruptions
-------------------------------------------------------------------*/

//Timer_A1 TACCR0 Interrupt Vector Handler Routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer1_A0(void)
{
    Result++;
//  Result = cpt;
//  cpt = 0;
    TA0CTL &= ~TAIFG;
}

//Timer_A1 TACCR1 Interrupt Vector Handler Routine
#pragma vector=TIMER1_A1_VECTOR
   __interrupt void Timer1_A1 (void)
{
    cpt++;
    TA1CTL &= ~TAIFG;       // Clear the interrupt flag
}
