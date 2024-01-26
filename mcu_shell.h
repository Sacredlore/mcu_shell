#ifndef MCU_SHELL_H
#define MCU_SHELL_H

#define _XTAL_FREQ 60000000UL

#pragma config FOSC      = HS1   // HS1 oscillator (Medium power, 4 MHz - 16 MHz)
//#pragma config FOSC      = HS2   // HS2 oscillator (High Power, 16 MHz - 25 MHz)
#pragma config PWRTEN    = ON    // Power Up Timer
#pragma config BOREN     = OFF   // Brown Out Detect (circuit that monitors the VDD level during operation)
#pragma config MCLRE     = ON    // Master Clear Enable (MCLR Enabled)
#pragma config RETEN     = ON    // Ultra low-power regulator is Enabled (Controlled by SRETEN bit)
#pragma config INTOSCSEL = LOW   // LF-INTOSC in Low-power mode during Sleep
#pragma config SOSCSEL   = DIG   // Digital (SCLKI) mode
#pragma config FCMEN     = OFF   // Fail-Safe Clock Monitor
#pragma config IESO      = OFF   // Internal External Oscillator Switch Over Mode
#pragma config WDTEN     = OFF   // Watchdog Timer
#pragma config CANMX     = PORTB // CANTX and CANRX pins are located on RB2 and RB3
#pragma config MSSPMSK   = MSK7  // MSSP address masking (7 Bit address masking mode)
#pragma config MCLRE     = ON    // Master Clear Enable (MCLR Enabled, RE3 Disabled)
#pragma config XINST     = OFF   // Extended Instruction Set disabled
#pragma config STVREN    = OFF   // Stack Overflow Reset (Disabled)
#pragma config PLLCFG    = ON    // PLL 4x enable bit

#pragma config CP0   = OFF // Code Protect 00800-03FFF (bitmask:0x01)
#pragma config CPB   = OFF // Code Protect Boot (bitmask:0x40)
#pragma config CPD   = OFF // Data EE Read Protect (bitmask:0x80)
#pragma config WRT0  = OFF // Table Write Protect 00800-03FFF (bitmask:0x01)
#pragma config WRT1  = OFF // Table Write Protect 04000-07FFF (bitmask:0x02)
#pragma config WRT2  = OFF // Table Write Protect 08000-0BFFF (bitmask:0x04)
#pragma config WRT3  = OFF // Table Write Protect 0C000-0FFFF (bitmask:0x08)
#pragma config WRTC  = OFF // Config. Write Protect (bitmask:0x20)
#pragma config WRTB  = OFF // Table Write Protect Boot (bitmask:0x40)
#pragma config WRTD  = OFF // Data EE Write Protect (bitmask:0x80)
#pragma config EBTR0 = OFF // Table Read Protect 00800-03FFF (bitmask:0x01)
#pragma config EBTR1 = OFF // Table Read Protect 04000-07FFF (bitmask:0x02)
#pragma config EBTR2 = OFF // Table Read Protect 08000-0BFFF (bitmask:0x04)
#pragma config EBTR3 = OFF // Table Read Protect 0C000-0FFFF (bitmask:0x08)
#pragma config EBTRB = OFF // Table Read Protect Boot (bitmask:0x40)
        
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strings and memory functions
#include <ctype.h> // character types
#include <stdint.h> // integer types
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h> // variadic functions
#include <math.h>
#include <float.h>

#define SET_BITMASK(a,b)   ((a) |= (b))
#define CLEAR_BITMASK(a,b) ((a) &= (~(b)))
#define SET_BIT(a,n)       ((a) |= (1ULL<<(n)))
#define CLEAR_BIT(a,n)     ((a) &= ~(1ULL<<(n)))
#define BIT_IS_SET(a,n)    (!!((a) & (1ULL<<(n))))
#define BIT_NOT_SET(a,n)   (!(BIT_IS_SET(a,n)))
#define VALUE_IS_BIT(a)    ((a == 0) || (a == 1))

/* timer0 */
#define TIMER0_PRELOAD_2500 0x06 // (  6) - 40000.00Hz,   0.00002500 sec     TIMER_FREQUENCY = (_XTAL_FREQ/4) / TIMER_PRESCALER / (256 - TIMER_PRELOAD);
#define TIMER0_PRELOAD_2000 0x38 // ( 56) - 50000.00Hz,   0.00002000 sec     TIMER_PERIOD = 1 / TIMER_FREQUENCY;
#define TIMER0_PRELOAD_1250 0x83 // (131) - 80000.00Hz,   0.00001250 sec
#define TIMER0_PRELOAD_1000 0x9C // (156) - 100000.00Hz,  0.00001000 sec
#define TIMER0_PRELOAD_0500 0xCE // (206) - 200000.00Hz,  0.00000500 sec
#define TIMER0_PRELOAD_0400 0xD8 // (216) - 250000.00Hz,  0.00000400 sec
#define TIMER0_PRELOAD_0250 0xE7 // (231) - 400000.00Hz,  0.00000250 sec
#define TIMER0_PRELOAD_0200 0xEC // (236) - 500000.00Hz,  0.00000200 sec
#define TIMER0_PRELOAD_0100 0xF6 // (246) - 1000000.00Hz, 0.00000100 sec
#define TIMER0_PRELOAD_0050 0xFb // (251) - 2000000.00Hz, 0.00000050 sec

#define TIMER0_PRESCALE_256 0b111 // 1:256 prescale value
#define TIMER0_PRESCALE_128 0b110 // 1:128 prescale value
#define TIMER0_PRESCALE_64  0b101 // 1:64 prescale value
#define TIMER0_PRESCALE_32  0b100 // 1:32 prescale value
#define TIMER0_PRESCALE_16  0b011 // 1:16 prescale value
#define TIMER0_PRESCALE_8   0b010 // 1:8 prescale value
#define TIMER0_PRESCALE_4   0b001 // 1:4 prescale value
#define TIMER0_PRESCALE_2   0b000 // 1:2 prescale value

/* timer1 */
#define TIMER1_PRESCALE_8 0b11 // 1:8 prescale value
#define TIMER1_PRESCALE_4 0b10 // 1:4 prescale value
#define TIMER1_PRESCALE_2 0b01 // 1:2 prescale value
#define TIMER1_PRESCALE_1 0b00 // 1:1 prescale value

/* timer2 */
#define TIMER2_PRESCALE_16 0b10 // 1:16 prescale value
#define TIMER2_PRESCALE_4  0b01 // 1:4 prescale value
#define TIMER2_PRESCALE_1  0b00 // 1:1 prescale value

#define TIMER2_POSTSCALE_16 0b0010 // 1:2 postscale value
#define TIMER2_POSTSCALE_8  0b0010 // 1:1 postscale value
#define TIMER2_POSTSCALE_4  0b0001 // 1:2 postscale value
#define TIMER2_POSTSCALE_1  0b0000 // 1:1 postscale value

/* refocon */
#define RODIV_32768 0b1111 // 0b1111
#define RODIV_16384 0b1110 // 0b1110
#define RODIV_8192  0b1101 // 0b1101
#define RODIV_4096  0b1100 // 0b1100
#define RODIV_2048  0b1011 // 0b1011
#define RODIV_1024  0b1010 // 0b1010
#define RODIV_512   0b1001 // 0b1001
#define RODIV_256   0b1000 // 0b1000
#define RODIV_128   0b0111 // 0b0111
#define RODIV_64    0b0110 // 0b0110
#define RODIV_32    0b0101 // 0b0101
#define RODIV_16    0b0100 // 0b0100
#define RODIV_8     0b0011 // 0b0011
#define RODIV_4     0b0010 // 0b0010
#define RODIV_2     0b0001 // 0b0001 - base clock value divided by 2
#define RODIV_0     0b0000 // 0b0000 - base clock value

/* sspm */
#define SSPM_8          0b1010 // 0b1010 - SPI Master mode: clock = FOSC/8
#define SSPM_SLAVE      0b0101 // 0b0101 - SPI Slave mode: clock = SCK pin; SS pin control disabled; SS can be used as I/O pin
#define SSPM_SLAVE_SS   0b0100 // 0b0100 - SPI Slave mode: clock = SCK pin; SS pin control enabled
#define SSPM_SLAVE_TMR2 0b0011 // 0b0011 - SPI Master mode: clock = TMR2 output/2
#define SSPM_64         0b0010 // 0b0010 - SPI Master mode: clock = FOSC/64
#define SSPM_16         0b0001 // 0b0001 - SPI Master mode: clock = FOSC/16
#define SSPM_4          0b0000 // 0b0000 - SPI Master mode: clock = FOSC/4

/* mcu_shell constants */
#define SERIAL_BUFFER_SIZE_MAX   0x40
#define SHELL_STRING_LENGTH_MAX  0x40
#define SHELL_COMMAND_LENGTH_MAX SHELL_STRING_LENGTH_MAX
#define SHELL_ARGUMENTS_MAX      0x15
#define SHELL_STACK_ARRAY_MAX    0x10

/* external variables */
extern char *msg_internal_restriction;
extern char *msg_stack_break;
extern char *msg_incorrect_arguments;
extern char *msg_command_not_found;
extern char *msg_unexpected_symbol;
extern char *msg_inconsistent_command;
extern char *msg_return_ok;

enum { /* shell states */
    SHELL_GET_INPUT,
    SHELL_PROCEED_COMMAND,
    SHELL_SHOW_PROMPT
};

enum { /* shell return codes */
    INTERNAL_RESTRICTION,
    STACK_BREAK,
    INCORRECT_ARGUMENTS,
    COMMAND_NOT_FOUND,
    UNEXPECTED_SYMBOL,
    INCONSISTENT_COMMAND,
    RETURN_OK
};

enum { /* frame machine states */
    FRAME_RECEIVE,
    FRAME_ERROR,
    FRAME_HANDLE,
    FRAME_VERIFY
};

enum { /* bytes in frame (for convenience) */
    FRAME_START, // CRC8
    FRAME_B1,    // select/write
    FRAME_B2,    // frequency/phase
    FRAME_B3,    // register
    FRAME_B4,    // h_msb
    FRAME_B5,    // h_lsb
    FRAME_B6,    // l_msb
    FRAME_B7,    // l_lsb
    FRAME_END    // CRC8
};

const uint8_t crc8_table[256] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
    0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
    0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
    0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
    0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
    0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
    0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
    0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
    0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
    0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
    0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
    0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
    0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

/* function prototypes */
uint8_t mcu_shell_echo(uint8_t argc, char *argv[]);
uint8_t mcu_shell_help(uint8_t argc, char *argv[]);
uint8_t mcu_shell_spi(uint8_t argc, char *argv[]);
uint8_t mcu_shell_glcd(uint8_t argc, char *argv[]);
uint8_t mcu_shell_dds(uint8_t argc, char *argv[]);
uint8_t mcu_shell_refocon(uint8_t argc, char *argv[]);
uint8_t mcu_shell_timer0(uint8_t argc, char *argv[]);
uint8_t mcu_shell_timer1(uint8_t argc, char *argv[]);
uint8_t mcu_shell_timer2(uint8_t argc, char *argv[]);
uint8_t mcu_shell_timer3(uint8_t argc, char *argv[]);
uint8_t mcu_shell_timer4(uint8_t argc, char *argv[]);
uint8_t mcu_shell_pmd(uint8_t argc, char *argv[]);
uint8_t mcu_shell_nrf(uint8_t argc, char *argv[]);
uint8_t build_argv(char cmdline[], uint8_t *argc, char *argv[]);
void mcu_shell_machine_command(void);
void mcu_shell_machine_silent(void);
bool mcu_setup_uart_advanced(uint32_t baud_rate);
bool mcu_setup_uart_compatible(uint32_t baud_rate);
uint8_t uart_transmit(char *bytes, uint8_t length);
uint8_t uart_receive(char *bytes, uint8_t length);
void uart_print_format(const char *format, ...);
uint8_t crc8(uint8_t bytes[], size_t length);

#endif
