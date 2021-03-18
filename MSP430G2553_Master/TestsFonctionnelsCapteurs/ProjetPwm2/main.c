/*-------------------------------------------------------------------
                    Declaration des #include
-------------------------------------------------------------------*/
#include <msp430.h> 
#include <stdio.h>
#include <string.h>

/*-------------------------------------------------------------------
                    Declaration des variables
-------------------------------------------------------------------*/
int choix;
int i;
/*-------------------------------------------------------------------
                    Declaration des fonctions
-------------------------------------------------------------------*/
void _Position1(){
    P2OUT |= BIT5;
    P2OUT |= BIT4;
}

void _Position2(){
    P2OUT &= ~BIT5;
    P2OUT |= BIT4;
}

void _Position3(){
    P2OUT &= ~ BIT5;
    P2OUT &= ~ BIT4;
}

void scan(){
    if(TA0CCR1 == 900){ // Si position basse (position initiale)
        while(TA0CCR1 != 2100){ // Mouvement vers le haut
            TA0CCR1 = TA0CCR1 + 10;
            __delay_cycles(5000);
        }
        while(TA0CCR1 != 900){ // Puis mouvement vers le bas
            TA0CCR1 = TA0CCR1 - 10;
            __delay_cycles(5000);
        }
    }
    while(!(TA1CTL & TAIFG));
    TA1CTL &= ~TAIFG; //RAZ TAIFG
}

void delai(){
    while(!(TA1CTL & TAIFG));
    TA1CTL &= ~TAIFG; //RAZ TAIFG
}

void pas_entier(){ //Fonction de déplacement en pas entier pour le moteur unipolaire avec Scan
    _Position3();
    scan();
    _Position2();
    scan();
    _Position3();
    scan();
    _Position1();
    scan();
}

void pas_entier_test(){ //Fonction de déplacement en pas entier pour le moteur unipolaire sans scan
    _Position3();
    delai();
    _Position2();
    delai();
    _Position3();
    delai();
    _Position1();
    delai();
}
void couple_max(){ //Fonction de déplacement en couple max pour le moteur unipolaire avec scan
    _Position2();
    scan();
    _Position2();
    scan();
    _Position1();
    scan();
    _Position1();
    scan();
}

void couple_max_test(){ //Fonction de déplacement en couple max pour le moteur unipolaire sans scan
    _Position2();
    delai();
    _Position2();
    delai();
    _Position1();
    delai();
    _Position1();
    delai();
}

void demi_pas(){ //Fonction de déplacement en demi pas pour le moteur unipolaire avec scan
    _Position3();
    scan();
    _Position2();
    scan();
    _Position2();
    scan();
    _Position2();
    scan();
    _Position3();
    scan();
    _Position1();
    scan();
    _Position1();
    scan();
    _Position1();
    scan();
}

void demi_pas_test(){ //Fonction de déplacement en demi pas pour le moteur unipolaire sans scan
    _Position3();
    delai();
    _Position2();
    delai();
    _Position2();
    delai();
    _Position2();
    delai();
    _Position3();
    delai();
    _Position1();
    delai();
    _Position1();
    delai();
    _Position1();
    delai();
}
/*-------------------------------------------------------------------
                    Declaration des interruptions
-------------------------------------------------------------------*/


#pragma vector = PORT1_VECTOR
__interrupt void detect_bouton(){ //Detection du bouton pour la mise en route du programme
    int i;
    switch(choix){
        case 0 : // test du PaP fonctionnement pas entier
            for(i=50;i>0;i--){
            pas_entier_test();
            }
            choix = 1;
            break;
       case 1 : // test du Pap fonctionnement couple max
            for(i=50;i>0;i--){
            couple_max_test();
            }
            choix = 2;
            break;
       case 2 : // test du PaP fonctionnement demi-pas
            for(i=50;i>0;i--){
            demi_pas_test();
            }
            choix = 3;
            break;
        case 3 : // Test du scan
            scan();
            choix = 4;
            break;
        case 4 : // Test de quelques scans pour le mode pas entier
            for(i=2;i>0;i--){
            pas_entier();
            }
            choix = 5;
            break;
       case 5 : // Test de quelques scans pour le mode couple max
            for(i=2;i>0;i--){
            couple_max();
            }
            choix = 6;
            break;
        case 6 : // Test de quelques scans pour le mode demi pas
            for(i=2;i>0;i--){
            demi_pas();
            }
            choix = 7;
            break;
        case 7 : // Test de scan complet en mode pas entier
            for(i=50;i>0;i--){
            pas_entier();
            }
            choix = 0;
            break;
    }
     P1IFG &= ~BIT3; //RAZ flag
}

/*-------------------------------------------------------------------
                  Programme principal
-------------------------------------------------------------------*/
void main(void){

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    BCSCTL1= CALBC1_1MHZ; //frequence de l'horloge 1MHz
    DCOCTL= CALDCO_1MHZ;
    //Constante
    choix = 0;
    //Entree du systeme - BP
    P1DIR &= ~BIT3;

    //Sortie du systeme
    P2DIR |= BIT5;  //Petite vitesse rotation
    P2DIR |= BIT4;  //Moyenne vitesse rotation

    P1DIR |= BIT6;  //Servomoteur
    P1SEL |= BIT6;
    P1SEL2 &= ~BIT6;

    //Mise en place du TIMER
    TA1CTL = 0|(TASSEL_2 | ID_1);   //source SMCLK, prediviseur par 8
    TA1CTL |= MC_3;                 //MODE UP/DOWN
    TA1CCR0 = 31250;                //temporisations de 500ms entre chaque pas

    //Initalisation du servomoteur
     TA0CTL = 0|(TASSEL_2 | ID_0);
     TA0CTL |= MC_1;
     TA0CCTL1 |= OUTMOD_7;
     TA0CCR0 = 20000;
     TA0CCR1 = 900;                 // initialisation en position basse

    //Mise en place de la detection du bouton
    P1REN |= BIT3;                  // activation resistance interne
    P1OUT |= BIT3;                  // mode pull-up
    P1IE |= BIT3;                   // activation interruption
    P1IES |= BIT3;                  // interruption surfront descendant

    //RAZ flag d'interruption
    P1IFG &= ~BIT3;
    __enable_interrupt();

}
