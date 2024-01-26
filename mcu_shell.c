#include "mcu_shell.h"
#include "analog.h"
#include "ad9832.h"
#include "nrf24l01.h"
#include "ili9341.h"

char *msg_internal_restriction = "internal restriction\n\r";
char *msg_stack_break = "stack break\n\r";
char *msg_incorrect_arguments = "incorrect arguments\n\r";
char *msg_command_not_found = "command not found\n\r";
char *msg_unexpected_symbol = "unexpected symbol\n\r";
char *msg_inconsistent_command = "inconsistent command\n\r";
char *msg_return_ok = "return ok\n\r";

#define SB_COUNT(sb) \
    (((sb).write - (sb).read) & (SERIAL_BUFFER_SIZE_MAX - 1))
#define SB_SPACE(sb) \
    ((sb).read - (((sb).write) + 1) & (SERIAL_BUFFER_SIZE_MAX - 1))

struct { /* transmit serial buffer */
    uint8_t read, write;
    char memory[SERIAL_BUFFER_SIZE_MAX];
} tx_sb = { 0, 0, 0 };

struct { /* receive serial buffer */
    uint8_t read, write;
    char memory[SERIAL_BUFFER_SIZE_MAX];
} rx_sb = { 0, 0, 0 };

struct { /* mcu_shell commands table */
    char *cmd_name, *description;
    uint8_t min_args, max_args;
    uint8_t(*cmd_pointer)(uint8_t argc, char *argv[]);
} shell_table[] = {
    { "help", "", 1, 1, mcu_shell_help},
    { "echo", "", 1, 5, mcu_shell_echo},
    { "spi", "", 3, 15, mcu_shell_spi},
    { "glcd", "", 2, 2, mcu_shell_glcd},
    { "dds", "", 4, 5, mcu_shell_dds},
    { "refocon", "", 3, 9, mcu_shell_refocon},
    { "timer0", "", 2, 21, mcu_shell_timer0},
    { "timer1", "", 3, 12, mcu_shell_timer1},
    { "timer2", "", 3, 15, mcu_shell_timer2},
    { "timer3", "", 1, 12, mcu_shell_timer3},
    { "timer4", "", 1, 15, mcu_shell_timer4},
    { "pmd", "", 3, 3, mcu_shell_pmd},
    { "nrf", "", 2, 3, mcu_shell_nrf},
    { "", "", 0, 0, ((void *) 0)}
};

uint8_t frame[] = { // sample frame
    187,  // CRC8
    0x01, // select/write
    0x02, // frequency/phase
    0x03, // register
    0x09, // h_msb
    0x09, // h_lsb
    0x09, // l_msb
    0x09, // l_lsb
    187   // CRC8
};

/* timers time */
uint16_t tmr0tm = 0;
uint16_t tmr1tm = 0;
uint16_t tmr2tm = 0;
uint16_t tmr3tm = 0;
uint16_t tmr4tm = 0;

uint8_t mcu_shell_help(uint8_t argc, char *argv[])
{
    uint8_t index = 0;

    for (;;) {
        if (strlen(shell_table[index].cmd_name)) {
            uart_print_format("%s\n\r", shell_table[index++].cmd_name);
        } else {
            break;
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_echo(uint8_t argc, char *argv[])
{
    for (uint8_t i = 1; i < argc; i++) {
        uart_print_format("%s\n\r", argv[i]);
    }

    return RETURN_OK;
}

uint8_t mcu_shell_spi(uint8_t argc, char *argv[])
{
    uint8_t spi_value = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "sspen")) {
            spi_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(spi_value)) {
                SSPCON1bits.SSPEN = spi_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "ckp")) {
            spi_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(spi_value)) {
                SSPCON1bits.CKP = spi_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "cke")) {
            spi_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(spi_value)) {
                SSPSTATbits.CKE = spi_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "smp")) {
            spi_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(spi_value)) {
                SSPSTATbits.SMP = spi_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "sspm")) {
            spi_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (spi_value) {
            case 4:
                SSPCON1bits.SSPM = SSPM_4; // clock = FOSC/4
                break;
            case 8:
                SSPCON1bits.SSPM = SSPM_8; // clock = FOSC/8
                break;
            case 16:
                SSPCON1bits.SSPM = SSPM_16; // clock = FOSC/16
                break;
            case 64:
                SSPCON1bits.SSPM = SSPM_64; // clock = FOSC/64
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_glcd(uint8_t argc, char *argv[])
{
    for (uint8_t i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "init")) {
            //
        }
        if (!strcmp(argv[i], "demo")) {
            //
        }
        if (!strcmp(argv[i], "clear")) {
            //
        }
        if (!strcmp(argv[i], "reset")) {
            //
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_dds(uint8_t argc, char *argv[])
{
    uint8_t dds_reg = 0;
    float dds_value = 0.0f;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "write")) {
            if (argc == 5) {
                dds_reg = (uint8_t) strtoll(argv[i + 2], (char **) NULL, 10);
                dds_value = strtof(argv[i + 3], (char **) NULL);
                if (!strcmp(argv[i + 1], "frequency")) {
                    switch (dds_reg) {
                    case 0:
                        write_freq0_register(dds_value);
                        return RETURN_OK;
                    case 1:
                        write_freq1_register(dds_value);
                        return RETURN_OK;
                    default:
                        return INCONSISTENT_COMMAND;
                    }
                } else if (!strcmp(argv[i + 1], "phase")) {
                    switch (dds_reg) {
                    case 0:
                        write_phase0_register(dds_value);
                        return RETURN_OK;
                    case 1:
                        write_phase1_register(dds_value);
                        return RETURN_OK;
                    case 2:
                        write_phase1_register(dds_value);
                        return RETURN_OK;
                    case 3:
                        write_phase3_register(dds_value);
                        return RETURN_OK;
                    default:
                        return INCONSISTENT_COMMAND;
                    }
                } else {
                    return INCONSISTENT_COMMAND;
                }
            } else {
                return INCONSISTENT_COMMAND;
            }
        } else if (!strcmp(argv[i], "select")) {
            if (argc == 4) {
                dds_reg = (uint8_t) strtoll(argv[i + 2], (char **) NULL, 10);
                if (!strcmp(argv[i + 1], "frequency")) {
                    switch (dds_reg) {
                    case 0:
                        bit_select_freq0_register();
                        return RETURN_OK;
                    case 1:
                        bit_select_freq1_register();
                        return RETURN_OK;
                    default:
                        return INCONSISTENT_COMMAND;
                    }
                } else if (!strcmp(argv[i + 1], "phase")) {
                    switch (dds_reg) {
                    case 0:
                        bit_select_phase0_register();
                        return RETURN_OK;
                    case 1:
                        bit_select_phase1_register();
                        return RETURN_OK;
                    case 2:
                        bit_select_phase2_register();
                        return RETURN_OK;
                    case 3:
                        bit_select_phase3_register();
                        return RETURN_OK;
                    default:
                        return INCONSISTENT_COMMAND;
                    }
                } else {
                    return INCONSISTENT_COMMAND;
                }
            } else {
                return INCONSISTENT_COMMAND;
            }
        } else {
            return INCONSISTENT_COMMAND;
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_refocon(uint8_t argc, char *argv[])
{
    uint8_t refocon_value = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "roon")) { // reference oscillator output enable bit
            refocon_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(refocon_value)) {
                REFOCONbits.ROON = refocon_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "rosel")) { // 0 - system clock is used as the base clock, 1 - primary oscillator (EC or HS) is used as the base clock
            refocon_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(refocon_value)) {
                REFOCONbits.ROSEL = refocon_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "rodiv")) { // reference oscillator divisor select bits
            refocon_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (refocon_value) {
            case 0:
                REFOCONbits.RODIV = RODIV_0; // base clock value
                break;
            case 2:
                REFOCONbits.RODIV = RODIV_2; // base clock value divided by 2
                break;
            case 4:
                REFOCONbits.RODIV = RODIV_4; // base clock value divided by 4
                break;
            case 8:
                REFOCONbits.RODIV = RODIV_8; // base clock value divided by 8
                break;
            case 16:
                REFOCONbits.RODIV = RODIV_16; // base clock value divided by 16
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_timer0(uint8_t argc, char *argv[])
{
    uint8_t timer0_value = 0;
    uint16_t timeval = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "tmr0on")) { // 0 - Stops Timer0, 1 - Enables Timer0
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer0_value)) {
                T0CONbits.TMR0ON = timer0_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "timeval")) { // set timer value
            timeval = (uint16_t) strtoll(argv[i + 1], (char **) NULL, 10);
            TMR0H = (uint8_t) timeval >> 8;
            TMR0L = (uint8_t) timeval;
        }
        if (!strcmp(argv[i], "t08bit")) { // 0 - Timer0 is configured as a 16-bit timer/counter, 1 - Timer0 is configured as an 8-bit timer/counter
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer0_value)) {
                T0CONbits.T08BIT = timer0_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t0cs")) { // 0 - Internal instruction cycle clock (CLKO), 1 - Transitions on T0CKI pin
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer0_value)) {
                T0CONbits.T0CS = timer0_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t0se")) { // Increments on low-to-high transition on T0CKI pin, 1 - Increments on high-to-low transition on T0CKI pin
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer0_value)) {
                T0CONbits.T0SE = timer0_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "psa")) { // 1 - Timer0 prescaler is not assigned, 0 - Timer0 prescaler is assigned
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer0_value)) {
                T0CONbits.PSA = timer0_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t0ps")) { // Timer0 Prescaler Select bits
            timer0_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer0_value) {
            case 2:
                T0CONbits.T0PS = TIMER0_PRESCALE_2;
                break;
            case 4:
                T0CONbits.T0PS = TIMER0_PRESCALE_4;
                break;
            case 8:
                T0CONbits.T0PS = TIMER0_PRESCALE_8;
                break;
            case 16:
                T0CONbits.T0PS = TIMER0_PRESCALE_16;
                break;
            case 32:
                T0CONbits.T0PS = TIMER0_PRESCALE_32;
                break;
            case 64:
                T0CONbits.T0PS = TIMER0_PRESCALE_64;
                break;
            case 128:
                T0CONbits.T0PS = TIMER0_PRESCALE_128;
                break;
            case 256:
                T0CONbits.T0PS = TIMER0_PRESCALE_256;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_timer1(uint8_t argc, char *argv[])
{
    uint8_t timer1_value = 0;
    uint16_t timeval = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "tmr1on")) { // 0 - Stops Timer1, 1 - Enables Timer1
            timer1_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer1_value)) {
                T1CONbits.TMR1ON = timer1_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "timeval")) { // set timer value
            timeval = (uint16_t) strtoll(argv[i + 1], (char **) NULL, 10);
            TMR1H = (uint8_t) timeval >> 8;
            TMR1L = (uint8_t) timeval;
        }
        if (!strcmp(argv[i], "tmr1cs")) { // 1 - Timer1 clock source is the system clock (FOSC), 0 - Timer1 clock source is the instruction clock (FOSC/4)
            timer1_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer1_value)) {
                T1CONbits.TMR1CS = timer1_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t1ckps")) { // Timer1 Input Clock Prescale Select bits
            timer1_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer1_value) {
            case 1:
                T1CONbits.T1CKPS = TIMER1_PRESCALE_1;
                break;
            case 2:
                T1CONbits.T1CKPS = TIMER1_PRESCALE_2;
                break;
            case 4:
                T1CONbits.T1CKPS = TIMER1_PRESCALE_4;
                break;
            case 8:
                T1CONbits.T1CKPS = TIMER1_PRESCALE_8;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_timer2(uint8_t argc, char *argv[])
{
    uint8_t timer2_value = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "tmr2on")) { // 0 - Stops Timer2, 1 - Enables Timer2
            timer2_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer2_value)) {
                T2CONbits.TMR2ON = timer2_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "tmr2")) { // Eight-bit Timer and Period registers (TMR2 and PR2, respectively)
            timer2_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            TMR2 = timer2_value;
        }
        if (!strcmp(argv[i], "pr2")) { // Eight-bit Timer and Period registers (TMR2 and PR2, respectively)
            timer2_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            PR2 = timer2_value;
        }
        if (!strcmp(argv[i], "t2ckps")) { // Timer2 Clock Prescale Select bits
            timer2_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer2_value) {
            case 1:
                T2CONbits.T2CKPS = TIMER2_PRESCALE_1;
                break;
            case 4:
                T2CONbits.T2CKPS = TIMER2_PRESCALE_4;
                break;
            case 16:
                T2CONbits.T2CKPS = TIMER2_PRESCALE_16;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t2outps")) { // Timer2 Output Postscale Select bits
            timer2_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer2_value) {
            case 1:
                T2CONbits.T2OUTPS = TIMER2_POSTSCALE_1;
                break;
            case 4:
                T2CONbits.T2OUTPS = TIMER2_POSTSCALE_4;
                break;
            case 8:
                T2CONbits.T2OUTPS = TIMER2_POSTSCALE_8;
                break;
            case 16:
                T2CONbits.T2OUTPS = TIMER2_POSTSCALE_16;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_timer3(uint8_t argc, char *argv[])
{

    return RETURN_OK;
}

uint8_t mcu_shell_timer4(uint8_t argc, char *argv[])
{
    uint8_t timer4_value = 0;

    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "tmr4on")) { // 0 - Stops Timer2, 1 - Enables Timer2
            timer4_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            if (VALUE_IS_BIT(timer4_value)) {
                T4CONbits.TMR4ON = timer4_value;
            } else {
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "tmr4")) { // Eight-bit Timer and Period registers (TMR4 and PR4, respectively)
            timer4_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            TMR4 = timer4_value;
        }
        if (!strcmp(argv[i], "pr4")) { // Eight-bit Timer and Period registers (TMR4 and PR4, respectively)
            timer4_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            PR4 = timer4_value;
        }
        if (!strcmp(argv[i], "t4ckps")) { // Timer4 Clock Prescale Select bits
            timer4_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer4_value) {
            case 1:
                T4CONbits.T4CKPS = TIMER2_PRESCALE_1;
                break;
            case 4:
                T4CONbits.T4CKPS = TIMER2_PRESCALE_4;
                break;
            case 16:
                T4CONbits.T4CKPS = TIMER2_PRESCALE_16;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
        if (!strcmp(argv[i], "t4outps")) { // Timer4 Output Postscale Select bits
            timer4_value = (uint8_t) strtoll(argv[i + 1], (char **) NULL, 10);
            switch (timer4_value) {
            case 1:
                T4CONbits.T4OUTPS = TIMER2_POSTSCALE_1;
                break;
            case 4:
                T4CONbits.T4OUTPS = TIMER2_POSTSCALE_4;
                break;
            case 8:
                T4CONbits.T4OUTPS = TIMER2_POSTSCALE_8;
                break;
            case 16:
                T4CONbits.T4OUTPS = TIMER2_POSTSCALE_16;
                break;
            default:
                return INCONSISTENT_COMMAND;
            }
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_pmd(uint8_t argc, char *argv[]) // peripheral module disable (PMD)
{
    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "disable")) {
            if (!strcmp(argv[i + 1], "adc")) {
                PMD1bits.ADCMD = 1;
                break;
            } else if (!strcmp(argv[i + 1], "can")) {
                PMD2bits.ECANMD = 1;
                break;
            } else if (!strcmp(argv[i + 1], "ctmu")) {
                PMD1bits.CTMUMD = 1;
                break;
            } else if (!strcmp(argv[i + 1], "uart2")) {
                PMD0bits.UART2MD = 1;
                break;
            } else {
                return INCONSISTENT_COMMAND;
            }
        } else if (!strcmp(argv[i], "enable")) {
            if (!strcmp(argv[i + 1], "adc")) {
                PMD1bits.ADCMD = 0;
                break;
            } else if (!strcmp(argv[i + 1], "can")) {
                PMD2bits.ECANMD = 0;
                break;
            } else if (!strcmp(argv[i + 1], "ctmu")) {
                PMD1bits.CTMUMD = 0;
                break;
            } else if (!strcmp(argv[i + 1], "uart2")) {
                PMD0bits.UART2MD = 0;
                break;
            } else {
                return INCONSISTENT_COMMAND;
            }
        } else {
            return INCONSISTENT_COMMAND;
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_nrf(uint8_t argc, char *argv[]) // nrf24l01 wireless transmitter/receiver
{
    for (uint8_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "print_registers")) {
            nrf_print_registers();
        } else if (!strcmp(argv[i], "print_config")) {
            nrf_print_config_register();
        } else if (!strcmp(argv[i], "print_fifo_status")) {
            nrf_print_fifo_status_register();
        } else if (!strcmp(argv[i], "print_pipes_address")) {
            nrf_print_pipes_address();
        } else if (!strcmp(argv[i], "override_pipes_address")) {
            nrf_override_pipes_address();
        } else {
            return INCONSISTENT_COMMAND;
        }
    }

    return RETURN_OK;
}

uint8_t uart_transmit(char *bytes, uint8_t length)
{
    uint8_t index = 0;

    do {
        if (SB_SPACE(tx_sb)) { // there is free space in transmit buffer
            tx_sb.memory[tx_sb.write++] = bytes[index++];
            tx_sb.write &= (SERIAL_BUFFER_SIZE_MAX - 1);
        } else {
            TX1IE = 1; // enable the transmit interrupt
        }
    } while (index < (length));
    
    if (SB_COUNT(tx_sb)) { // nothing should be left in transmit buffer
        TX1IE = 1;
    }

    return index;
}

uint8_t uart_receive(char *bytes, uint8_t length)
{
    uint8_t index = 0;

    do {
        if (SB_COUNT(rx_sb)) { // there is something to receive in buffer
            bytes[index++] = rx_sb.memory[rx_sb.read++];
            rx_sb.read &= (SERIAL_BUFFER_SIZE_MAX - 1);
        } else {
            break;
        }
    } while (index < (length));

    return index;
}

void uart_print_format(const char *format, ...)
{
    va_list ap;
    char string[SHELL_STRING_LENGTH_MAX];

    va_start(ap, format);
    vsprintf(string, format, ap);
    uart_transmit(string, (uint8_t) strlen(string));
    va_end(ap);
}

void __interrupt(high_priority) mcu_interrupt_high(void)
{
    if (HLVDIE && HLVDIF) {
        HLVDIF = 0;
    } else if (TMR0IE && TMR0IF) {
        ++tmr0tm;
        LATBbits.LATB4 = 1;
        TMR0IF = 0;
    } else if (TMR1IE && TMR1IF) {
        ++tmr1tm;
        LATBbits.LATB5 = 1;
        TMR1IF = 0;
    } else if (TMR2IE && TMR2IF) {
        ++tmr2tm;
        LATBbits.LATB6 = 1;
        TMR2IF = 0;
    } else if (TMR3IE && TMR3IF) {
        ++tmr3tm;
        TMR3IF = 0;
    } else if (TMR4IE && TMR4IF) {
        ++tmr4tm;
        LATBbits.LATB7 = 1;
        TMR4IF = 0;
    } else if (INT0IE && INT0IF) {
        INT0IF = 0;
    }
}

void __interrupt(low_priority) mcu_interrupt_low(void)
{
    if (ADIE && ADIF) {
        ADIF = 0;
    } else if (CTMUIE && CTMUIF) {
        CTMUIF = 0;
    } else if (INT1IE && INT1IF) {
        INT1IF = 0;
    } else if (INT2IE && INT2IF) {
        INT2IF = 0;
    } else if (INT3IE && INT3IF) {
        INT3IF = 0;
    } else if (TX1IE && TX1IF) {
		if (SB_COUNT(tx_sb)) { // interrupt turns off itself
			TXREG1 = tx_sb.memory[tx_sb.read++];
			tx_sb.read &= (SERIAL_BUFFER_SIZE_MAX - 1);
		} else {
            while (TRMT1 == 0) // wait for last byte transmits
                continue;
            TX1IE = 0;
		}        
    } else if (RC1IE && RC1IF) {
        if (RCSTA1bits.FERR1) {
            __asm("movf RCREG1,W"); // discard on framing error
        }
        if (RCSTA1bits.OERR1) {
            __asm("bcf RCSTA1,4"); // reset the receiver (CREN)
            __asm("bsf RCSTA1,4");
        }
        if (SB_SPACE(rx_sb)) {
            rx_sb.memory[rx_sb.write++] = RCREG1;
            rx_sb.write &= (SERIAL_BUFFER_SIZE_MAX - 1);
        } else {
            rx_sb.memory[rx_sb.write] = RCREG1; // overwrite
        }
    }
}

uint8_t crc8(uint8_t bytes[], size_t length) {
	size_t i, j;
	uint8_t crc = 0xFF;

	for (i = 0; i < length; i++) {
		crc ^= bytes[i];
		for (j = 0; j < 8; j++) {
			if ((crc & 0x80) != 0)
				crc = (uint8_t) ((crc << 1) ^ 0x31);
			else
				crc <<= 1;
		}
	}

	return crc;
}

uint8_t crc8_fast(uint8_t bytes[], size_t length) {
	size_t i;
	uint8_t crc = 0xFF;

	for (i = 0; i < length; i++) {
		crc = crc8_table[crc ^ bytes[i]];
	}

	return crc;
}

uint8_t build_argv(char *cmd_line, uint8_t *argc, char *argv[])
{
    uint8_t id_stack = 0, id_memlc = 0;
    char h_stack[SHELL_STACK_ARRAY_MAX]; // history stack
    char *h_memlc[SHELL_STACK_ARRAY_MAX]; // locations in memory

    memset(h_stack, 0, sizeof (h_stack));
    memset(h_memlc, 0, sizeof (h_memlc));
    do {
        if (isspace(*cmd_line)) { // is space symbol or tab
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = 's'; // overwrite
                h_memlc[id_stack] = cmd_line;
                break;
            case 's':
                break; // do nothing
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1)
                    return STACK_BREAK;
                h_stack[id_stack] = 's';
                h_memlc[id_stack] = cmd_line;
                break;
            }
        } else if (isalnum(*cmd_line)) { // is letter or digit
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = 'l'; // overwrite
                h_memlc[id_stack] = cmd_line;
                break;
            case 'l':
                break; // do nothing
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1)
                    return STACK_BREAK;
                h_stack[id_stack] = 'l';
                h_memlc[id_stack] = cmd_line;
                break;
            }
        } else if (ispunct(*cmd_line)) { // is punctuation character
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = *cmd_line; // overwrite
                h_memlc[id_stack] = cmd_line;
                break;
            case '.':
                break; // do nothing
            case 'l':
                break; // do nothing
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1)
                    return STACK_BREAK;
                h_stack[id_stack] = *cmd_line;
                h_memlc[id_stack] = cmd_line;
                break;
            }
        } else { // not space, not alphanumeric, not punctuation, not end of string
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = *cmd_line; // overwrite
                h_memlc[id_stack] = cmd_line;
                break;
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1)
                    return STACK_BREAK;
                h_stack[id_stack] = *cmd_line;
                h_memlc[id_stack] = cmd_line;
                break;
            }
        }
    } while (*cmd_line++);

    do {
        switch (h_stack[id_memlc]) {
        case 'l':
            argv[(*argc)++] = h_memlc[id_memlc++];
            break;
        case 's':
            *h_memlc[id_memlc++] = '\0';
            break;
        case '\0': // stack end
            break;
        default:
            return UNEXPECTED_SYMBOL;
        }
    } while (id_stack--);

    return RETURN_OK;
}

void mcu_shell_exec(char *cmd_line)
{
    uint8_t argc = 0;
    char *argv[SHELL_ARGUMENTS_MAX];
    uint8_t ecode, index = 0;

    memset(argv, 0, sizeof (argv));
    ecode = build_argv(cmd_line, &argc, argv);
    if (ecode == RETURN_OK) {
        ecode = COMMAND_NOT_FOUND; // expect that the command not found
        while (strlen(shell_table[index].cmd_name)) {
            if (strcmp(argv[0], shell_table[index].cmd_name)) {
                index++;
            } else {
                if (argc < shell_table[index].min_args || argc > shell_table[index].max_args) {
                    ecode = INCORRECT_ARGUMENTS;
                } else {
                    ecode = (*shell_table[index].cmd_pointer)(argc, argv);
                }
                break;
            }
        }
    }

    switch (ecode) {
    case INTERNAL_RESTRICTION:
        uart_transmit(msg_internal_restriction, (uint8_t) strlen(msg_internal_restriction));
        break;
    case STACK_BREAK:
        uart_transmit(msg_stack_break, (uint8_t) strlen(msg_stack_break));
        break;
    case INCORRECT_ARGUMENTS:
        uart_transmit(msg_incorrect_arguments, (uint8_t) strlen(msg_incorrect_arguments));
        break;
    case COMMAND_NOT_FOUND:
        uart_transmit(msg_command_not_found, (uint8_t) strlen(msg_command_not_found));
        break;
    case UNEXPECTED_SYMBOL:
        uart_transmit(msg_unexpected_symbol, (uint8_t) strlen(msg_unexpected_symbol));
        break;
    case INCONSISTENT_COMMAND:
        uart_transmit(msg_inconsistent_command, (uint8_t) strlen(msg_inconsistent_command));
        break;
    case RETURN_OK:
        uart_transmit(msg_return_ok, (uint8_t) strlen(msg_return_ok));
        break;
    }
}

void mcu_shell_machine_command(void)
{
    const char bell_character = '\a';
    const char *shell_prompt = "\nmcu_shell# ";
    bool echo_rx = true, show_prompt = true;
    static uint8_t command_shell_state = SHELL_SHOW_PROMPT;
    static uint8_t cmd_memory_offset = 0;
    static char cmd_memory[SHELL_COMMAND_LENGTH_MAX];

    switch (command_shell_state) {
    case SHELL_GET_INPUT:
        while (uart_receive(&cmd_memory[cmd_memory_offset], 1)) { // receive one character
            switch (cmd_memory[cmd_memory_offset]) {
            case '\r': // carriage return (0x0d)
                if (echo_rx)
                    uart_transmit(&cmd_memory[cmd_memory_offset], 1);
                cmd_memory[cmd_memory_offset] = '\0';
                if (cmd_memory_offset > 0) { // proceed command
                    command_shell_state = SHELL_PROCEED_COMMAND;
                } else { // no command
                    command_shell_state = SHELL_SHOW_PROMPT;
                }
                break;
            case '\b': // proteus terminal transmits backspace (0x08)
                if (cmd_memory_offset > 0) { // cmd_memory has something to delete
                    if (echo_rx)
                        uart_transmit(&cmd_memory[cmd_memory_offset], 1);
                    cmd_memory_offset--;
                } else {
                    if (echo_rx)
                        uart_transmit(&bell_character, 1);
                }
                break;
            case 0x7f: // putty transmits backspace
                if (cmd_memory_offset > 0) { // cmd_memory has something to delete
                    if (echo_rx)
                        uart_transmit(&cmd_memory[cmd_memory_offset], 1);
                    cmd_memory_offset--;
                } else {
                    if (echo_rx)
                        uart_transmit(&bell_character, 1);
                }
                break;
            case 0x03: // putty transmits Ctrl-C
                break;
            case 0x1a: // putty transmits pause key
                break;
            case 0x1b: // escape sequence start
                // [0x1b][0x5b][0x33][0x7e] - delete key                
                // [0x1b][0x5b][0x35][0x7e] - home key
                // [0x1b][0x5b][0x36][0x7e] - end key
                // [0x1b][0x5b][0x31][0x7e] - page up
                // [0x1b][0x5b][0x34][0x7e] - page down
                // [0x1b][0x5b][0x32][0x7e] - insert key                
                // [0x1b][0x5b][0x44] - arrow left
                // [0x1b][0x5b][0x43] - arrow right
                // [0x1b][0x5b][0x41] - arrow up
                // [0x1b][0x5b][0x42] - arrow left
                // [0x1b][0x5b][0x31][0x31][0x7e] F1 key
                // [0x1b][0x5b][0x31][0x32][0x7e] F2 key
                // [0x1b][0x5b][0x31][0x33][0x7e] F3 key
                // [0x1b][0x5b][0x31][0x34][0x7e] F3 key
                break;
            default:
                if (cmd_memory_offset >= sizeof (cmd_memory)) { // cmd_memory hits its limit
                    if (echo_rx)
                        uart_transmit(&bell_character, 1);
                } else {
                    if (echo_rx)
                        uart_transmit(&cmd_memory[cmd_memory_offset], 1);
                    cmd_memory_offset++;
                }
            }
        }
        break;
    case SHELL_PROCEED_COMMAND:
        mcu_shell_exec(cmd_memory);
        cmd_memory_offset = 0;
        command_shell_state = SHELL_SHOW_PROMPT;
        break;
    case SHELL_SHOW_PROMPT:
        if (show_prompt)
            uart_transmit(shell_prompt, (uint8_t) strlen(shell_prompt));
        command_shell_state = SHELL_GET_INPUT;
        break;
    }
}

void mcu_shell_machine_silent(void)
{
    static uint8_t silent_shell_state = SHELL_PROCEED_COMMAND;
    static uint8_t cmd_memory_offset;
    static char cmd_memory[SHELL_COMMAND_LENGTH_MAX];

    switch (silent_shell_state) {
    case SHELL_PROCEED_COMMAND:
        while (uart_receive(&cmd_memory[cmd_memory_offset], 1)) { // receive one character
            switch (cmd_memory[cmd_memory_offset]) {
            case 'w':
                //
                break;
            case 'a':
                //
                break;
            case 's':
                //
                break;
            case 'd':
                //
                break;
            }

            if (cmd_memory_offset >= sizeof (cmd_memory)) { // cmd_memory hits its limit
                cmd_memory_offset = 0;
            } else {
                cmd_memory_offset++;
            }
            silent_shell_state = SHELL_PROCEED_COMMAND;
        }
    }
}

void mcu_shell_machine_frame(void) {
    static uint8_t frame_memory[sizeof (frame)];
    static uint8_t frame_index = 0;
    static uint8_t machine_state = FRAME_RECEIVE;
    static uint8_t crc_start = 0, crc_end = 0;

    uint8_t select_write;
    uint8_t frequency_phase;
    uint8_t dds_register;
    uint32_t dds_value;

    switch (machine_state) {
        case FRAME_RECEIVE:
            if (uart_receive(&frame_memory[frame_index], 1)) {
                switch (frame_index) {
                    case FRAME_START:
                        crc_start = frame_memory[frame_index];
                        break;
                    case FRAME_END:
                        crc_end = frame_memory[frame_index];
                        machine_state = FRAME_VERIFY;
                        break;
                }
                frame_index = ((frame_index + 1) % sizeof (frame)); // circular index
            }
            break;
        case FRAME_VERIFY:
            if ((crc_start != crc_end)
                    && (crc_end != crc8(&frame[FRAME_B1], FRAME_END - FRAME_B1))) {
                machine_state = FRAME_ERROR;
            } else {
                machine_state = FRAME_HANDLE;
            }
            break;
        case FRAME_ERROR:
            machine_state = FRAME_RECEIVE;
            draw_text("FRAME_ERROR", GLCD_FONT_8X8, 10, 10);
            LATB5 = 1;
            break;
        case FRAME_HANDLE:
            //draw_text("FRAME_HANDLE", GLCD_FONT_8X8, 10, 10);
            LATB4 = 1;
            select_write = (uint8_t) frame_memory[FRAME_B1];
            frequency_phase = (uint8_t) frame_memory[FRAME_B2];
            dds_register = (uint8_t) frame_memory[FRAME_B3];
            dds_value = (uint32_t) frame_memory[FRAME_B4] << 24;
            dds_value |= (uint32_t) frame_memory[FRAME_B5] << 16;
            dds_value |= (uint32_t) frame_memory[FRAME_B6] << 8;
            dds_value |= (uint32_t) frame_memory[FRAME_B7];
            machine_state = FRAME_RECEIVE;
            
            char s[256];
            
            //sprintf(s, "%d %d %d", select_write, frequency_phase, dds_register);
            
            sprintf(s, "%u", select_write);
            label1_set_text(s); 
            sprintf(s, "%u", frequency_phase);
            label2_set_text(s);
            sprintf(s, "%u", dds_register);
            label3_set_text(s);
            sprintf(s, "%lu", dds_value);
            label4_set_text(s);
            sprintf(s, "%u", crc_end);
            label5_set_text(s);
            
            break;
    }
}

bool mcu_setup_uart_advanced(uint32_t baud_rate) // UART1: TRISC6 - 0, TRISC7 - 1, UART2: TRISB6 - 0, TRISB7 - 1
{
    int32_t spbrg;

    spbrg = (int32_t) ((_XTAL_FREQ / (baud_rate * 16)) - 1); // baud rate generator low
    if ((spbrg > 0) && (spbrg < UINT16_MAX)) {
        BRG161 = 1;
        BRGH1 = 0;
        TXEN1 = 1;
        CREN1 = 1;
        SPEN1 = 1;
        SPBRGH1 = (uint8_t) (spbrg >> 8), SPBRG1 = (uint8_t) spbrg;
        return true;
    } else {
        spbrg = (int32_t) ((_XTAL_FREQ / (baud_rate * 4)) - 1); // baud rate generator high
        if ((spbrg > 0) && (spbrg < UINT16_MAX)) {
            BRG161 = 1;
            BRGH1 = 1;
            TXEN1 = 1;
            CREN1 = 1;
            SPEN1 = 1;
            SPBRGH1 = (uint8_t) (spbrg >> 8), SPBRG1 = (uint8_t) spbrg;
            return true;
        } else {
            return false; // baud rate not in range
        }
    }
}

bool mcu_setup_uart_compatible(uint32_t baud_rate) // UART1: TRISC6 - 0, TRISC7 - 1, UART2: TRISB6 - 0, TRISB7 - 1
{
	int32_t spbrg;

    spbrg = (int32_t) ((_XTAL_FREQ / (baud_rate * 64)) - 1); // baud rate generator low
    if ((spbrg > 0) && (spbrg < UINT8_MAX)) {
        BRGH1 = 0;
        TXEN1 = 1;
        CREN1 = 1;
        SPEN1 = 1;
        SPBRG1 = (uint8_t) spbrg;
        return true;
    } else {
        spbrg = (int32_t) ((_XTAL_FREQ / (baud_rate * 16)) - 1); // baud rate generator high
        if ((spbrg > 0) && (spbrg < UINT8_MAX)) {
            BRGH1 = 1;
            TXEN1 = 1;
            CREN1 = 1;
            SPEN1 = 1;
            SPBRG1 = (uint8_t) spbrg;
            return true;
        } else {
            return false; // baud rate not in range
        }
    }
}

void mcu_setup_timer0(void)
{
    T0CONbits.TMR0ON = 0; // 0 - Stops Timer0, 1 - Enables Timer0
    T0CONbits.T08BIT = 1; // 0 - Timer0 is configured as a 16-bit timer/counter, 1 - Timer0 is configured as an 8-bit timer/counter
    T0CONbits.T0CS = 0; // 0 - Internal instruction cycle clock (CLKO), 1 - Transitions on T0CKI pin
    T0CONbits.T0SE = 0; // 0 - Increments on low-to-high transition on T0CKI pin, 1 - Increments on high-to-low transition on T0CKI pin
    T0CONbits.PSA = 1; // 1 - Timer0 prescaler is not assigned, 0 - Timer0 prescaler is assigned
    T0CONbits.T0PS = 0b000; // 0b000 - 1:2 prescale value, 0b001 - 1:4 prescale value, 0b010 - 1:8 prescale value
    TMR0H = 0, TMR0L = 0x06; // timer_time = (uint16_t)((TMR0H << 8) | TMR0L);
    //TMR0H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR0L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //    
    TMR0ON = 0; // 1 - Enables Timer0, 0 - Stops Timer0
}

void mcu_setup_timer1(void)
{
    T1GCONbits.TMR1GE = 0; // 0 - Timer1 counts regardless of Timer1 gate function, 1 - Timer1 counting is controlled by the Timer1 gate function
    T1GCONbits.T1GPOL = 0; // 1 - Timer1 gate is active-high (Timer1 counts when gate is high), 0 - Timer1 gate is active-low (Timer1 counts when gate is low)
    T1GCONbits.T1GTM = 0; // 1 - Timer1 Gate Toggle mode is enabled, 0 - Timer1 Gate Toggle mode is disabled and toggle flip-flop is cleared
    T1GCONbits.T1GSPM = 0; // 1 - Timer1 Gate Single Pulse mode is enabled and is controlling Timer1 gate, 0 - Timer1 Gate Single Pulse mode is disabled
    T1GCONbits.T1GVAL = 0; // Indicates the current state of the Timer1 gate that could be provided to TMR1H:TMR1L; unaffected by Timer1 Gate Enable (TMR1GE) bit
    T1GCONbits.T1GSS = 0; // Timer1 Gate Source Select bits, 0b11 - Comparator 2 output, 0b10 - Comparator 1 output, 0b01 - TMR2 to match PR2 output, 0b00 - Timer1 gate pin    

    T1CONbits.TMR1ON = 0; // 0 - Stops Timer1, 1 - Enables Timer1
    T1CONbits.TMR1CS = 0; // 1 - Timer1 clock source is the system clock (FOSC), 0 - Timer1 clock source is the instruction clock (FOSC/4)
    T1CONbits.T1CKPS = 0; // 0 - 1:1 Prescale value
    T1CONbits.SOSCEN = 0; // 0 - SOSC is disabled for Timer1, 1 - SOSC is enabled and available for Timer1
    T1CONbits.NOT_T1SYNC = 0; // 1 - Do not synchronize external clock input, 0 -  Synchronizes external clock input
    T1CONbits.RD16 = 1; // 1 - Enables register read/write of Timer1 in one 16-bit operation, 0 - Enables register read/write of Timer1 in two 8-bit operations
    TMR1H = 0, TMR1L = 0; // timer_time = (uint16_t)((TMR1H << 8) | TMR1L);
    //TMR1H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR1L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //
    TMR1ON = 0; // 1 - Enables Timer1, 0 - Stops Timer1
}

void mcu_setup_timer2(void)
{
    T2CONbits.T2OUTPS = 0; // 0b0000 - 1:1 Postscale, 0b0001 - 1:2 Postscale, ... 0b1111 - 1:16 Postscale
    T2CONbits.T2CKPS = 0; // 0b00 - Prescaler is 1, 0b01 - Prescaler is 4, 0b1x - Prescaler is 16
    TMR2 = 0, PR2 = 0xFF; // Eight-bit Timer and Period registers (TMR2 and PR2, respectively)
    T2CONbits.TMR2ON = 0; // 1 - Enables Timer2, 0 - Stops Timer2
}

void mcu_setup_timer3(void)
{
    T3GCONbits.TMR3GE = 0; // 0 - Timer1 counts regardless of Timer1 gate function, 1 - Timer1 counting is controlled by the Timer1 gate function
    T3GCONbits.T3GPOL = 0; // 1 - Timer1 gate is active-high (Timer1 counts when gate is high), 0 - Timer1 gate is active-low (Timer1 counts when gate is low)
    T3GCONbits.T3GTM = 0; // 1 - Timer1 Gate Toggle mode is enabled, 0 - Timer1 Gate Toggle mode is disabled and toggle flip-flop is cleared
    T3GCONbits.T3GSPM = 0; // 1 - Timer1 Gate Single Pulse mode is enabled and is controlling Timer1 gate, 0 - Timer1 Gate Single Pulse mode is disabled
    T3GCONbits.T3GVAL = 0; // Indicates the current state of the Timer1 gate that could be provided to TMR1H:TMR1L; unaffected by Timer1 Gate Enable (TMR1GE) bit
    T3GCONbits.T3GSS = 0; // Timer1 Gate Source Select bits, 0b11 - Comparator 2 output, 0b10 - Comparator 1 output, 0b01 - TMR2 to match PR2 output, 0b00 - Timer1 gate pin    

    T3CONbits.TMR3ON = 0; // 0 - Stops Timer3, 1 - Enables Timer3
    T3CONbits.TMR3CS = 0; // 1 - Timer1 clock source is the system clock (FOSC), 0 - Timer1 clock source is the instruction clock (FOSC/4)
    T3CONbits.T3CKPS = 0; // 0 - 1:1 Prescale value
    T3CONbits.SOSCEN = 0; // 0 - SOSC is disabled for Timer1, 1 - SOSC is enabled and available for Timer1
    T3CONbits.NOT_T3SYNC = 0; // 1 - Do not synchronize external clock input, 0 -  Synchronizes external clock input
    T3CONbits.RD16 = 1; // 1 - Enables register read/write of Timer1 in one 16-bit operation, 0 - Enables register read/write of Timer1 in two 8-bit operations
    TMR3H = 0, TMR3L = 0; // timer_time = (uint16_t)((TMR3H << 8) | TMR3L);
    //TMR3H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR3L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //
    TMR3ON = 0; // 1 - Enables Timer3, 0 - Stops Timer3
}

void mcu_setup_timer4(void)
{
    T4CONbits.T4OUTPS = 0; // 0b0000 - 1:1 Postscale, 0b0001 - 1:2 Postscale, ... 0b1111 - 1:16 Postscale
    T4CONbits.T4CKPS = 0; // 0b00 - Prescaler is 1, 0b01 - Prescaler is 4, 0b1x - Prescaler is 16
    TMR4 = 0, PR4 = 0xFF; // Eight-bit Timer and Period registers (TMR4 and PR4, respectively)
    T4CONbits.TMR4ON = 0; // 1 - Enables Timer4, 0 - Stops Timer4
}

void mcu_setup_hlvd()
{
    HLVDCONbits.HLVDEN = 1;
    HLVDCONbits.VDIRMAG = 0; // 1 - voltage equals or exceeds trip point, 0 - voltage equals or falls below trip point
    HLVDCONbits.HLVDL = 0b1000; // 0b1000 - voltage in range ~(3.24V > 3.32V < 3.41V), 0b1110 - voltage in range ~(4.64V > 4.75V < 4.87V)
    while (!HLVDCONbits.BGVST)
        NOP();

    while (!HLVDCONbits.IRVST)
        NOP();
}

void mcu_setup_cpp_modules()
{
    CCP1CONbits.CCP1M = 0b0000; // EECCP

    CCP2CONbits.CCP2M = 0b0000; // CCP
    CCP3CONbits.CCP3M = 0b0000;
    CCP4CONbits.CCP4M = 0b0000;
    CCP5CONbits.CCP5M = 0b0000;

    CCPTMRSbits.C1TSEL = 0;
    CCPTMRSbits.C2TSEL = 0;
    CCPTMRSbits.C3TSEL = 0;
    CCPTMRSbits.C4TSEL = 0;
    CCPTMRSbits.C5TSEL = 0;
}

void mcu_setup_spi(void) // TRISC5 - 0 (SDO), TRISC4 - 1 (SDI), TRISC3 - 0 (SCK))
{
    ODCONbits.SSPOD = 0; // disable open drain output
    SSPCON1bits.SSPM = SSPM_4;
    SSPCON1bits.CKP = 0; // clock polarity select bit
    SSPSTATbits.CKE = 1; // clock edge select
    SSPSTATbits.SMP = 1; // sample bit
    SSPCON1bits.SSPEN = 1;
}

void mcu_setup_interrupts(void)
{
    IPEN = 1, PEIE = 0; // enable interrupt priority (1 - high priority, 0 - low priority)

    HLVDIP = 1, HLVDIF = 0, HLVDIE = 0; // High/Low-Voltage Detect module (HLVD)
    TMR0IP = 1, TMR0IF = 0, TMR0IE = 1; // timers
    TMR1IP = 1, TMR1IF = 0, TMR1IE = 1;
    TMR2IP = 1, TMR2IF = 0, TMR2IE = 1;
    TMR3IP = 1, TMR3IF = 0, TMR3IE = 1;
    TMR4IP = 1, TMR4IF = 0, TMR4IE = 1;
    TX1IP = 0, TX1IF = 0, TX1IE = 0; // Universal Asynchronous Receiver-Transmitter (UART)
    RC1IP = 0, RC1IF = 0, RC1IE = 1;
    TX2IP = 0, TX2IF = 0, TX2IE = 0;
    RC2IP = 0, RC2IF = 0, RC2IE = 1;
    ADIP = 0, ADIF = 0, ADIE = 0; // A/D converter
    SSPIP = 0, SSPIF = 0, SSPIE = 0; // MSSP
    CTMUIP = 0, CTMUIF = 0, CTMUIE = 0; // CTMU
    RBIP = 0, RBIE = 0, RBIF = 0; // interrupt-on-change
    INT0IF = 0, INTEDG0 = 0, INT0IE = 0;
    INT1IP = 0, INT1IF = 0, INTEDG1 = 0, INT1IE = 0; // external interrupts
    INT2IP = 0, INT2IF = 0, INTEDG2 = 0, INT2IE = 0;
    INT3IP = 0, INT3IF = 0, INTEDG3 = 0, INT3IE = 0;

    GIEH = 1, GIEL = 1; // high and low priority interrupts
    GIE = 1; // global interrupts enable
}

void main(void)
{
    TRISA = 0b00000000, LATA = 0b00000000, SLRA = 0b00000000; // TRIS (data direction) 0 - output, 1 - input
    TRISB = 0b00000000, LATB = 0b00000000, SLRB = 0b00000000; // LAT (output level) 0 - low, 1 - high
    TRISC = 0b10000000, LATC = 0b10000001, SLRC = 0b00000000; // SLR (output pins slew rate) 1 - slew at 0.1 the standard rate, 0 - slew at standard rate

    ANCON0 = 0b00000000, ANCON1 = 0b00000000; // ANCON (ports analog/digital functionality) 0 - digital, 1 - analog

    //mcu_setup_hlvd();
    mcu_setup_spi();
    //mcu_setup_uart_advanced(9600);
    mcu_setup_uart_compatible(9600);
    //mcu_setup_analog_systems();
    mcu_setup_interrupts();
    //mcu_setup_timer0();
    //mcu_setup_timer1();
    //mcu_setup_timer2();
    //mcu_setup_timer3();
    //mcu_setup_timer4();

    initialize_glcd();
    //glcd_clear_screen(GLCD_BACKGROUND_COLOR);
    rectangle glcd_rect = {0, 0, 319, 239};
    initialize_gui(glcd_rect);
    glcd_demo();

    REFOCONbits.ROSEL = 0; // 0 - system clock is used as the base clock, 1 - primary oscillator (EC or HS) is used as the base clock
    REFOCONbits.ROSSLP = 0; // 0 - reference oscillator is disabled in sleep mode
    REFOCONbits.RODIV = RODIV_2;
    REFOCONbits.ROON = 0;
 
//    __delay_ms(10);
//    dds_circuit_power_up();
//    write_freq0_register(1.000f);
//    bit_select_freq0_register();

    for (;;) { // main loop
        //mcu_shell_machine_command();
        //mcu_shell_machine_silent();
        mcu_shell_machine_frame();
        __delay_us(100);
    }
}
