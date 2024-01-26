#ifndef AD9832_H
#define AD9832_H

/* software SPI */
#define SF_SPI_SCK  LATCbits.LATC4 // SCK, pin 15
#define SF_SPI_SDO  LATCbits.LATC5 // SDO, pin 16
#define SF_SPI_SDI  PORTBbits.RB0  // SDI, pin 21

/* register selection pins */
#define FSYNC   LATBbits.LATB5 // pin 26 (SPI chip select)
#define FSELECT LATCbits.LATC2 // pin 13 (select frequency register)
#define PSEL0   LATCbits.LATC0 // pin 11 (select phase register)
#define PSEL1   LATCbits.LATC1 // pin 12 (select phase register)

/* SPI chip select */
#define dds_circuit_select()    FSYNC = 0
#define dds_circuit_unselect()  FSYNC = 1

/* reference frequency (MHz) */
#define MCLK_FREQUENCY 25.0f

/* control commands */
#define CMD_CONTROL_SYNC_SELRC      0x8000
#define CMD_CONTROL_SLEEP_RESET_CLR 0xC000

/* control commands bits */
#define SYNC_BIT  0x2000
#define SELRC_BIT 0x1000 // when SELSRC = 0, the pins are used, and when SELRC = 1 the bits are used
#define SLEEP_BIT 0x2000
#define RESET_BIT 0x1000
#define CLR_BIT   0x0800

/* registers related commands */
#define CMD_WRITE_16_BITS_PHASE_REG      0x0000
#define CMD_WRITE_8_PHASE_BITS_DEFER_REG 0x1000
#define CMD_WRITE_16_BITS_FREQ_REG       0x2000
#define CMD_WRITE_8_FREQ_BITS_DEFER_REG  0x3000
#define CMD_SELECT_PHASE_REG             0x4000
#define CMD_SELECT_FREQ_REG              0x5000
#define CMD_SELECT_PHASE_FREQ_REG        0x6000

/* register selection bit masks */
#define FREQ0_SELECT  0x0000
#define FREQ1_SELECT  0x0800
#define PHASE0_SELECT 0x0000
#define PHASE1_SELECT 0x0200
#define PHASE2_SELECT 0x0400
#define PHASE3_SELECT 0x0600

/* registers */
#define FREQ0_L_LSB      0x0000
#define FREQ0_H_LSB      0x0100
#define FREQ0_L_MSB      0x0200
#define FREQ0_H_MSB      0x0300
#define FREQ1_L_LSB      0x0400
#define FREQ1_H_LSB      0x0500
#define FREQ1_L_MSB      0x0600
#define FREQ1_H_MSB      0x0700
#define PHASE0_LSB       0x0800
#define PHASE0_MSB       0x0900
#define PHASE1_LSB       0x0A00
#define PHASE1_MSB       0x0B00
#define PHASE2_LSB       0x0C00
#define PHASE2_MSB       0x0D00
#define PHASE3_LSB       0x0E00
#define PHASE3_MSB       0x0F00

/* function prototypes */
uint8_t sf_spi_write_byte(uint8_t byte);
uint16_t sf_spi_write_word(uint16_t word);
uint8_t hw_spi_write_byte(uint8_t byte);
uint16_t hw_spi_write_word(uint16_t word);
void dds_circuit_power_down(void);
void dds_circuit_power_up(void);
void dds_circuit_power_up_no_sync_no_selrc(void);
void dds_circuit_power_up_no_sync(void); // after write to frequency or phase registers the changes will occur a little faster
void dds_circuit_power_up_no_selrc(void); // the pins are used to switch frequency and phase registers
void write_freq0_register(float frequency);
void write_freq1_register(float frequency);
void write_phase0_register(float phase);
void write_phase1_register(float phase);
void write_phase2_register(float phase);
void write_phase3_register(float phase);

/* function similar macro */
#define bit_select_freq0_register()  sf_spi_write_word(CMD_SELECT_FREQ_REG | FREQ0_SELECT)
#define bit_select_freq1_register()  sf_spi_write_word(CMD_SELECT_FREQ_REG | FREQ1_SELECT)
#define bit_select_phase0_register() sf_spi_write_word(CMD_SELECT_PHASE_REG | PHASE0_SELECT)
#define bit_select_phase1_register() sf_spi_write_word(CMD_SELECT_PHASE_REG | PHASE1_SELECT)
#define bit_select_phase2_register() sf_spi_write_word(CMD_SELECT_PHASE_REG | PHASE2_SELECT)
#define bit_select_phase3_register() sf_spi_write_word(CMD_SELECT_PHASE_REG | PHASE3_SELECT)

#define pin_select_freq0_register()  FSELECT = 0
#define pin_select_freq1_register()  FSELECT = 1
#define pin_select_phase0_register() PSEL1 = 0, PSEL0 = 0
#define pin_select_phase1_register() PSEL1 = 0, PSEL0 = 1
#define pin_select_phase2_register() PSEL1 = 1, PSEL0 = 0
#define pin_select_phase3_register() PSEL1 = 1, PSEL0 = 1

#endif
