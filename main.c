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

//main
int main(void) {
    //config USART
    PORTA_DIR = 0x40; // TX out
    PORTA_PIN6CTRL = 0X08; //enable r pull-up
    PORTA_PIN7CTRL = 0X08; //enable r pull-up
    USART0_BAUD = 0X056D; //9600 bauds
    USART0_CTRLC = 0x03; //8 bit data, 1 stop bit, without parity
    USART0_CTRLB = 0XC0; //enable transmit and receive
    //vars for loop
    unsigned char data[6], rxcmd;
    int i2c;
    //main loop
    while(1) {        
        while(!(USART0_STATUS & USART_RXCIF_bm));
        rxcmd = USART0_RXDATAL;
        if (rxcmd == 0x31) {
            i2c = sht40(data, 0xFD); 
            if (i2c == 0) {
                USART0_TXDATAL = 'e';
                while(!(USART0_STATUS & USART_DREIF_bm));
            } else {
                for (int i = 0; i < 6; i++) {
                    USART0_TXDATAL = (data[i] >> 4) + 48;
                    while(!(USART0_STATUS & USART_DREIF_bm));
                    USART0_TXDATAL = (data[i] & 0x0F) + 48;
                    while(!(USART0_STATUS & USART_DREIF_bm));
                    //USART0_TXDATAL = 0x20; send space
                    //while(!(USART0_STATUS & USART_DREIF_bm));
                }
            }
        }
    }
}