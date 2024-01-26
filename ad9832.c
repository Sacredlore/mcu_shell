#include "mcu_shell.h"
#include "ad9832.h"

uint8_t hw_spi_write_byte(uint8_t byte)
{
    dds_circuit_select();
    SSPBUF = byte; // put byte into SPI buffer
    while (!SSPSTATbits.BF) // wait for the transfer to finish
        NOP();
    dds_circuit_unselect();

    return SSPBUF; // save the read value
}

uint16_t hw_spi_write_word(uint16_t word)
{
    uint16_t value;

    dds_circuit_select();
    SSPBUF = (word >> 8); // most significant byte
    while (!SSPSTATbits.BF) // wait for the transfer to finish
        NOP();
    value = SSPBUF;
    SSPBUF = (word & 0xFF); // least significant byte
    while (!SSPSTATbits.BF) // wait for the transfer to finish
        NOP();
    value = (value << 8 | SSPBUF);
    dds_circuit_unselect();

    return value;
}

uint8_t sf_spi_write_byte(uint8_t byte)
{
    uint8_t value = 0; // accept that there are only zeros from SPI
    uint8_t mask = 0x80; // set mask to write and read bit 7 (0x80 = 128)

    dds_circuit_select();
    do {
        if (byte & mask) // clock out current bit to SPI
            SF_SPI_SDO = 1;
        else
            SF_SPI_SDO = 0;
        SF_SPI_SCK = 1; // set SPI clock line and delay
        __delay_us(10);
        if (SF_SPI_SDI)
            value |= mask; // set bit from SPI
        SF_SPI_SCK = 0; // clear SPI clock line and delay
        __delay_us(10);
        mask = mask >> 1; // shift mask to next bit
    } while (mask != 0);
    dds_circuit_unselect();

    return value;
}

uint16_t sf_spi_write_word(uint16_t word)
{
    uint16_t value = 0; // accept that there are only zeros from SPI
    uint16_t mask = 0x8000; // set mask to write and read bit 15 (0x8000 = 32768)

    dds_circuit_select();
    do {
        if (word & mask) // clock out current bit to SPI
            SF_SPI_SDO = 1;
        else
            SF_SPI_SDO = 0;
        SF_SPI_SCK = 1; // set SPI clock line and delay
        __delay_us(10);
        if (SF_SPI_SDI)
            value |= mask; // set bit from SPI
        SF_SPI_SCK = 0; // clear SPI clock line and delay
        __delay_us(10);
        mask = mask >> 1; // shift mask to next bit
    } while (mask != 0);
    dds_circuit_unselect();

    return value;
}

void dds_circuit_power_down(void)
{
    sf_spi_write_word(CMD_CONTROL_SLEEP_RESET_CLR | SLEEP_BIT | RESET_BIT | CLR_BIT);
}

void dds_circuit_power_up(void)
{
    sf_spi_write_word(CMD_CONTROL_SLEEP_RESET_CLR);
    sf_spi_write_word(CMD_CONTROL_SYNC_SELRC | SYNC_BIT | SELRC_BIT);
}

void dds_circuit_power_up_no_sync_no_selrc(void)
{
    sf_spi_write_word(CMD_CONTROL_SLEEP_RESET_CLR);
    sf_spi_write_word(CMD_CONTROL_SYNC_SELRC);
}

void dds_circuit_power_up_no_sync(void)
{
    sf_spi_write_word(CMD_CONTROL_SLEEP_RESET_CLR);
    sf_spi_write_word(CMD_CONTROL_SYNC_SELRC | SELRC_BIT);
}

void dds_circuit_power_up_no_selrc(void)
{
    sf_spi_write_word(CMD_CONTROL_SLEEP_RESET_CLR);
    sf_spi_write_word(CMD_CONTROL_SYNC_SELRC | SYNC_BIT);
}

void write_freq0_register(float frequency)
{
    uint32_t dds_freq = (uint32_t) ((frequency / MCLK_FREQUENCY) * 0xFFFFFFFF); // frequency in MHz

    uint8_t h_msb = (uint8_t) (dds_freq >> 24);
    uint8_t l_msb = (uint8_t) (dds_freq >> 16);
    uint8_t h_lsb = (uint8_t) (dds_freq >> 8);
    uint8_t l_lsb = (uint8_t) (dds_freq);

    sf_spi_write_word(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ0_H_MSB | h_msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_FREQ_REG | FREQ0_L_MSB | l_msb);
    sf_spi_write_word(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ0_H_LSB | h_lsb);
    sf_spi_write_word(CMD_WRITE_16_BITS_FREQ_REG | FREQ0_L_LSB | l_lsb);
}

void write_freq1_register(float frequency)
{
    uint32_t dds_freq = (uint32_t) ((frequency / MCLK_FREQUENCY) * 0xFFFFFFFF); // frequency in MHz

    uint8_t h_msb = (uint8_t) (dds_freq >> 24);
    uint8_t l_msb = (uint8_t) (dds_freq >> 16);
    uint8_t h_lsb = (uint8_t) (dds_freq >> 8);
    uint8_t l_lsb = (uint8_t) (dds_freq);

    sf_spi_write_word(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ1_H_MSB | h_msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_FREQ_REG | FREQ1_L_MSB | l_msb);
    sf_spi_write_word(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ1_H_LSB | h_lsb);
    sf_spi_write_word(CMD_WRITE_16_BITS_FREQ_REG | FREQ1_L_LSB | l_lsb);
}

void write_phase0_register(float phase)
{
    uint16_t dds_phase = (uint16_t) (((phase / (M_PI * 2.0f)) * 4096.0f)); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    sf_spi_write_word(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE0_MSB | msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_PHASE_REG | PHASE0_LSB | lsb);
}

void write_phase1_register(float phase)
{
    uint16_t dds_phase = (uint16_t) (((phase / (M_PI * 2.0f)) * 4096.0f)); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    sf_spi_write_word(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE1_MSB | msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_PHASE_REG | PHASE1_LSB | lsb);
}

void write_phase2_register(float phase)
{
    uint16_t dds_phase = (uint16_t) (((phase / (M_PI * 2.0f)) * 4096.0f)); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    sf_spi_write_word(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE2_MSB | msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_PHASE_REG | PHASE2_LSB | lsb);
}

void write_phase3_register(float phase)
{
    uint16_t dds_phase = (uint16_t) (((phase / (M_PI * 2.0f)) * 4096.0f)); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    sf_spi_write_word(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE3_MSB | msb);
    sf_spi_write_word(CMD_WRITE_16_BITS_PHASE_REG | PHASE3_LSB | lsb);
}
