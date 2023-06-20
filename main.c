/*
Convert i2c data from sht40 to serial data with attiny202
author: izmahel
date: 07/jul/2022
device: attiny202
*/

//constants
#define F_CPU 3333333UL

//libraries
#include <math.h>
#include <util/delay.h>
#include <avr/io.h>

//global vars

//functions
//crc calculator
unsigned char get_crc8_2_bytes(unsigned char *data) {
    unsigned char crc = 0xFF;
    for (int i = 0; i < 2; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++){
            if ((crc & 0x80) != 0) {
                crc = (unsigned char)((crc << 1) ^ 0x31);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc; 
}
//read i2c sht40 with address 0x44
int sht40(unsigned char *data, unsigned char cmd) {
    unsigned char t[2], rh[2], crct, crcrh;
     // config TWI
    TWI0_MBAUD = 0x0A; //100kHz
    TWI0_MCTRLA = 0x01; //on master twi
    TWI0_MCTRLB = TWI_ACKACT_ACK_gc; //send ack
    TWI0_MSTATUS = 0x01; // to idle status
    // write command
    TWI0_MADDR = 0x88; //address 0x44 + write
    while(!(TWI0_MSTATUS & TWI_WIF_bm)); //while to complete transfer
    TWI0_MDATA = cmd; //command
    while(!(TWI0_MSTATUS & TWI_WIF_bm)); //while to complete transfer
    TWI0_MCTRLB = TWI_MCMD_STOP_gc; //stop condition
    _delay_ms(10);
    TWI0_MADDR = 0x89; //address 0x44+read bit
    for (int i = 0; i < 6; i++) {
        while(!(TWI0_MSTATUS & TWI_RIF_bm)); //while to complete transfer
        data[i] = TWI0_MDATA;
        TWI0_MCTRLB = TWI_MCMD_RECVTRANS_gc; //ack
    }
    TWI0_MCTRLB = TWI_MCMD_STOP_gc; //stop condition
    TWI0_MCTRLA = 0x00; //off master twi
    // validate CRC
    t[0] = data[0];
    t[1] = data[1];
    rh[0] = data[3];
    rh[1] = data[4];
    crct = get_crc8_2_bytes(t);
    crcrh = get_crc8_2_bytes(rh);
    if ((crct == data[2]) && (crcrh == data[5])) return 1;
    else return 0;
}
void to_idle(void){
    int i=0;
    char rst;
    while(1) {
        while(!(USART0_STATUS & USART_RXCIF_bm));
        rst = USART0_RXDATAL;
        if (rst == 0x66) i++;
        else i = 0;
        if (i == 3) break;
    }
}

//main
int main(void) {
    //config USART
    USART0_BAUD = 0X056D; //9600 bauds
    //USART0_BAUD = 0X0ADA; //4800 bauds
    //USART0_CTRLA = 0X01; //enable rs485 control
    USART0_CTRLB = 0X80; //enable rx
    USART0_CTRLC = 0x03; //8 bit data, 1 stop bit, without parity
    //config ports
    //PORTA_PIN0CTRL = 0X08; //enable r pull-up
    PORTA_PIN1CTRL = 0X08; //enable r pull-up
    PORTA_PIN2CTRL = 0X08; //enable r pull-up
    PORTA_PIN3CTRL = 0X08; //enable r pull-up
    PORTA_PIN6CTRL = 0X08; //enable r pull-up
    PORTA_PIN7CTRL = 0X08; //enable r pull-up
    //config port to eneable max485
    PORTA_DIR = 0x08;
    PORTA_OUT &= 0xF7;
    //vars for loop
    unsigned char data[6], s, addr, cmd, rxcmd;
    int i2c;
    //main loop
    while(1) {        
        while(!(USART0_STATUS & USART_RXCIF_bm));
        s = USART0_RXDATAL;
        //start command
        if (s == 0x55) {
            while(!(USART0_STATUS & USART_RXCIF_bm));
            addr = USART0_RXDATAL;
            //if the address is for this device send data 
            if (addr == 0x31) {
                //receive command
                while(!(USART0_STATUS & USART_RXCIF_bm));
                rxcmd = USART0_RXDATAL;
                if (rxcmd == 0xF1) cmd = 0xFD;
                else if (rxcmd == 0xF2) cmd = 0xF6;
                else if (rxcmd == 0xF3) cmd = 0xE0;
                else if (rxcmd == 0xF4) cmd = 0x89;
                else if (rxcmd == 0xF5) cmd = 0x94;
                else if (rxcmd == 0xF6) cmd = 0x39;
                else if (rxcmd == 0xF7) cmd = 0x32;
                else if (rxcmd == 0xF8) cmd = 0x2F;
                else if (rxcmd == 0xF9) cmd = 0x24;
                else if (rxcmd == 0xFA) cmd = 0x1E;
                else if (rxcmd == 0xFB) cmd = 0x15;
                else cmd = 0;
                if (cmd != 0) {
                    //confirm reicevied comand
                    PORTA_DIR = 0x48; // TX out
                    USART0_CTRLB = 0xC0; //enable tx
                    PORTA_OUT |= 0x08;
                    //_delay_ms(1);
                    //USART0_TXDATAL = 0x55;
                    //while(!(USART0_STATUS & USART_DREIF_bm));

                    // read rh and t
                    i2c = sht40(data, cmd);
                    // send data
                    if (i2c == 0) {
                        for (int i = 0; i < 6; i++) {
                            USART0_TXDATAL = 'e';
                            while(!(USART0_STATUS & USART_DREIF_bm));
                        }
                    } else {
                        for (int j = 0; j < 6; j++) {
                            
                            USART0_TXDATAL = data[j];
                            while(!(USART0_STATUS & USART_DREIF_bm));
                            
                            /*
                            USART0_TXDATAL = (data[i] >> 4) + 48;
                            while(!(USART0_STATUS & USART_DREIF_bm));
                            USART0_TXDATAL = (data[i] & 0x0F) + 48;
                            while(!(USART0_STATUS & USART_DREIF_bm));
                            */
                        }
                    }
                    //while(!(USART0_STATUS & USART_TXCIF_bm));
                    _delay_ms(4);
                    PORTA_OUT &= 0xF7;
                    USART0_CTRLB = 0x80; //disable tx
                }
                //after send data go to idle
                to_idle();
            } 
            //if the address isn't for this device going to idle state
            else {
                to_idle();
            }   
        }
    }
}