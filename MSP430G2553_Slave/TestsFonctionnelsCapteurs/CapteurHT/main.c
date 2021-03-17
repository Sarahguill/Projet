#include <msp430.h> 
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/***********************************
 * Define et déclarations
 ***********************************/
#define CMDLEN  12               // longueur maximum de la commande utilisateur
#define LF      0x0A
#define CR      0x0D

#define PMODHYGRO_ADDR  0x40
#define SCL BIT6
#define SDA BIT7

#define CMD_TEMP_REG_SLAVE     0x00
#define CMD_HUMR_REG_SLAVE     0x01
#define CMD_CONF_REG_SLAVE     0x02
#define CONF_RST               BITF
#define CONF_HEAT              BITD
#define CONF_MODE              BITC
#define CONF_BTST              BITB
#define CONF_TRES              BITA
#define CONF_HRES              (BIT9 | BIT8)

#define TYPE_0_LENGTH          1
#define TYPE_1_LENGTH          2

int MasterType1[TYPE_1_LENGTH] = {0, 1};
int MasterType0[TYPE_0_LENGTH] = {0};
int SlaveType1[TYPE_1_LENGTH] = {0, 1};
int SlaveType0[TYPE_0_LENGTH] = {0};

/* ReceiveBuffer: Buffer used to receive data in the ISR
* RXByteCtr: Number of bytes left to receive
* ReceiveIndex: The index of the next byte to be received in ReceiveBuffer
* TransmitBuffer: Buffer used to transmit data in the ISR
* TXByteCtr: Number of bytes left to transfer
* TransmitIndex: The index of the next byte to be transmitted in TransmitBuffer
* */

int ReceiveBuffer[10] = {0};
int RXByteCtr = 0;
int ReceiveIndex = 0;
int TransmitBuffer[10] = {0};
int TXByteCtr = 0;
int TransmitIndex = 0;

/***********************************
 * Machine d'état
 ***********************************/
typedef enum I2C_ModeEnum
{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITHC_TO_TX_MODE,
    TIMEOUT_MODE
} I2C_Mode;

/* Used to track the state of the software state machine */
I2C_Mode MasterMode = IDLE_MODE;

/* The Register Address or Command to use */
int TransmitRegAddr = 0x00;

/***********************************
 * Appel aux fonctions
 ***********************************/
void Init_IO(void);
void InitUART(void);
void RXdata(unsigned char *c);
void TXdata(unsigned char c);
void Send_STR_UART(const char *msg);
void command( char *cmd );

void init_I2C(void);
I2C_Mode I2C_Master_WriteReg(int dev_addr, int reg_addr, int *reg_data, int count);

I2C_Mode I2C_Master_ReadReg(int, int, int);
//void CopyArray(int, int, int);

void CopyArray(int *source, int *dest, int count);


/***********************************
 * Déclaration variables globales
 ***********************************/
volatile int Humidity;
volatile int Temperature;

/***********************************
 * main.c
 ***********************************/
int main(void)
{
    unsigned char c;
    char  cmd[CMDLEN];      /* tableau de caractere lie a la commande user */
    int   nb_car;           /* compteur nombre carateres saisis */

    WDTCTL = WDTPW + WDTHOLD;   /* Stop WDT - clock calibration verification */

    if(CALBC1_1MHZ==0xFF || CALDCO_1MHZ==0xFF)
        {
        __low_power_mode_4();
        }
    /* factory calibration parameters */
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    Init_IO();
    InitUART();

    nb_car = 0;
    Send_STR_UART("MSP430 Ready !");

    I2C_Master_ReadReg(PMODHYGRO_ADDR, CMD_TEMP_REG_SLAVE, TYPE_0_LENGTH);
    CopyArray(ReceiveBuffer, SlaveType0, TYPE_0_LENGTH);
    __bis_SR_register(LPM0_bits | GIE);

    /*
    while(1)
    {
      if( nb_car<(CMDLEN-1) )
      {
        RXdata(&c);
        if( (cmd[(unsigned int)nb_car]=c) != CR )
        {
            TXdata(c);
            nb_car++;
        }
        else
        {
          cmd[(unsigned int)nb_car]=0x00;
          command(cmd);
          nb_car=0;
        }
      }
    }
    */
}
//******************************************************************************
// I2C Interrupt For Received and Transmitted Data *****************************
//******************************************************************************
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
    if (IFG2 & UCB0RXIFG)                 // Receive Data Interrupt
    {
        //Must read from UCB0RXBUF
        int rx_val = UCB0RXBUF;
        if (RXByteCtr){
            ReceiveBuffer[ReceiveIndex++] = rx_val;
            RXByteCtr--;
        }

        if (RXByteCtr == 1){
            UCB0CTL1 |= UCTXSTP;
        }
        else if (RXByteCtr == 0){
            IE2 &= ~UCB0RXIE;
            MasterMode = IDLE_MODE;
            __bic_SR_register_on_exit(LPM0_bits);      // Exit LPM0
            }
    }
    else if (IFG2 & UCB0TXIFG)            // Transmit Data Interrupt
    {
        switch (MasterMode)
        {
        case TX_REG_ADDRESS_MODE:
            UCB0TXBUF = TransmitRegAddr;
            if (RXByteCtr){
                MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
            }
            else {
                MasterMode = TX_DATA_MODE;        // Continue to transmision with the data in Transmit Buffer
            }
            break;

        case SWITCH_TO_RX_MODE:
            IE2 |= UCB0RXIE;              // Enable RX interrupt
            IE2 &= ~UCB0TXIE;             // Disable TX interrupt
            UCB0CTL1 &= ~UCTR;            // Switch to receiver
            MasterMode = RX_DATA_MODE;    // State state is to receive data
            UCB0CTL1 |= UCTXSTT;          // Send repeated start

            if (RXByteCtr == 1)
            {
                //Must send stop since this is the N-1 byte
                while((UCB0CTL1 & UCTXSTT));
                UCB0CTL1 |= UCTXSTP;      // Send stop condition
                }
            break;

        case TX_DATA_MODE:
            if (TXByteCtr)
            {
                UCB0TXBUF = TransmitBuffer[TransmitIndex++];
                TXByteCtr--;
            }
            else {
                //Done with transmission
                UCB0CTL1 |= UCTXSTP;     // Send stop condition
                MasterMode = IDLE_MODE;
                IE2 &= ~UCB0TXIE;                       // disable TX interrupt
                __bic_SR_register_on_exit(LPM0_bits);      // Exit LPM0
            }
            break;

        default:
            __no_operation();
            break;
        }
    }
 }

//******************************************************************************
// I2C Interrupt For Start, Restart, Nack, Stop ********************************
//******************************************************************************
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{
    if (UCB0STAT & UCNACKIFG)
    {
        UCB0STAT &= ~UCNACKIFG;             // Clear NACK Flags
    }
    if (UCB0STAT & UCSTPIFG)                //Stop or NACK Interrupt
    {
        UCB0STAT &= ~(UCSTTIFG | UCSTPIFG | UCNACKIFG); //Clear START/STOP/NACK
    }
    if (UCB0STAT & UCSTTIFG)
    {
        UCB0STAT &= ~(UCSTTIFG);                    //Clear START Flags
    }
}

/***********************************
 * Fonctions
 ***********************************/

/***********************************
 * Mise en place communication UART
 ***********************************/

void Init_IO(void)
{
   P1DIR |= BIT0 | BIT6;        /* port 1.0  en sortie */
   P1OUT &= ~(BIT0 | BIT6);     /* force etat bas P1.0 - LED1 */
   P1REN |= BIT6;
}

void InitUART(void)
{
    P1SEL |= (BIT1 | BIT2);                     /* P1.1 = RXD, P1.2=TXD */
    P1SEL2 |= (BIT1 | BIT2);                    /* P1.1 = RXD, P1.2=TXD */
    UCA0CTL1 = UCSWRST;                         /* SOFTWARE RESET */
    UCA0CTL1 |= UCSSEL_3;                       /* SMCLK (2 - 3) */

    UCA0CTL0 &= ~(UCPEN | UCMSB | UCDORM);
    UCA0CTL0 &= ~(UC7BIT | UCSPB | UCMODE_3 | UCSYNC);  /* dta:8 stop:1 usci_mode3uartmode */
    UCA0CTL1 &= ~UCSWRST;                               /* **Initialize USCI state machine** */

    UCA0BR0 = 104;                              /* 1MHz, OSC16, 9600 (8Mhz : 52) : 8/115k */
    UCA0BR1 = 0;                                /* 1MHz, OSC16, 9600 */
    UCA0MCTL = 10;

    /* Enable USCI_A0 RX interrupt */
    IE2 |= UCA0RXIE;
}

void RXdata(unsigned char *c)
{
    while (!(IFG2&UCA0RXIFG));              /* buffer Rx USCI_A0 plein ? */
    *c = UCA0RXBUF;
}

void TXdata(unsigned char c)
{
    while (!(IFG2&UCA0TXIFG));              /* buffer Tx USCI_A0 vide ? */
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
/***********************************
 * Mise en place commandes
 ***********************************/

void command( char *cmd ){
    init_I2C();
    if(strcmp(cmd, "Temp") == 0)
    {
        P1OUT |= BIT0;
        Send_STR_UART("\rTemperature mesuree\n");
        while (1)
        {
            /* Lecture de la Temperature */
            Temperature = MSP430_getTemp();
        }
        // Send_STR_UART ("\rTemperature ");
        printf("%d", Temperature);
        P1OUT &= ~BIT0;
    }
    else if(strcmp(cmd, "Hum") == 0)
    {
        P1OUT |= BIT0;
        Send_STR_UART("\rHumidite mesuree\n");
        while (1)
        {
            /* Lecture de la Humidité */
            Humidity = lMSP430_getHum();
        }
        // Send_STR_UART ("\rHumidite");
        printf("%d", Humidity);
        P1OUT &= ~BIT0;
    }
    else
    {
        Send_STR_UART("\rMauvaise commande ");
        Send_STR_UART("\rEntrez 'Temp' pour Temperature\n");
        Send_STR_UART("\rEntrez 'Hum' pour Humidite\n");
    }
}

/***********************************
 * Mise en place communication I2C
 ***********************************/
void init_I2C(void) {
    UCB0CTL1 |= UCSWRST;                            /* Enable SW reset */
    UCB0CTL1 = 0 | (UCSSEL_2 | UCSWRST);            /* Use SMCLK, keep SW reset */
    P1SEL  |= (SCL | SDA);                          /* Assign I2C pins to USCI_B0 */
    P1SEL2 |= (SCL | SDA);                          /* Datasheet SLAS735J page 43 */

    P1REN |= (BIT6 | BIT7);         /* Enable resistor */
    P1OUT |= (BIT6 | BIT7);         /* PULL-UP */

    UCB0CTL0 = 0 | (UCMST | UCMODE_3 | UCSYNC);     /* I2C Master, synchronous mode */
    UCB0BR0 = 0X0A;                                 /* fSCL = SMCLK/10 = ~100kHz */
    UCB0BR1 = 0;
    UCB0I2COA = 0x77;                           /* Own Address */
    UCB0I2CSA = PMODHYGRO_ADDR;                 /* Slave Address */
    UCB0CTL1 &= ~UCSWRST;                       /* Clear SW reset, resume operation */
    UCB0I2CIE |= UCNACKIE;
}

I2C_Mode I2C_Master_WriteReg(int dev_addr, int reg_addr, int *reg_data, int count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;

    //Copy register data to TransmitBuffer
    CopyArray(reg_data, TransmitBuffer, count);
    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    IFG2 &= ~(UCB0TXIFG | UCB0RXIFG);       // Clear any pending interruptS
    IE2 &= ~UCB0RXIE;                       // Disable RX interrupt
    IE2 |= UCB0TXIE;                        // Enable TX interrupt
    UCB0CTL1 |= (UCTR | UCTXSTT);           // I2C TX, start condition
    __bis_SR_register(LPM0_bits | GIE);     // Enter LPM0 w/ interrupts

    return MasterMode;
}

I2C_Mode I2C_Master_ReadReg(int dev_addr, int reg_addr, int count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    IFG2 &= ~(UCB0TXIFG | UCB0RXIFG);       // Clear any pending interrupts
    IE2 &= ~UCB0RXIE;                       // Disable RX interrupt
    IE2 |= UCB0TXIE;                        // Enable TX interrupt

    UCB0CTL1 |= (UCTR | UCTXSTT);           // I2C TX, start condition
    __bis_SR_register(LPM0_bits | GIE);        // Enter LPM0 w/ interrupts

    return MasterMode;
}

/*
 * R/W buffer
 */
void CopyArray(int *source, int *dest, int count)
{
    int copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++)
    {
        dest[copyIndex] = source[copyIndex];
    }
}
