#include "mcu_shell.h"
#include "analog.h"

float internal_capacitance = 0.0f;
float offset = 0.0f, gain_error = 1.0f, offset_voltage = 0.0f, superfluous_voltage = 0.0f;
float ctmu_current_0_55_uA = 0.0f, ctmu_current_5_5_uA = 0.0f, ctmu_current_55_uA = 0.0f;
float mcu_vdd = 0.0f, full_charge_voltage = 0.0, discharge_voltage = 0.0f;

uint8_t mcp_spi_send(uint8_t byte) // CKP - 0, CKE - 1, SMP - 1
{
    SSPBUF = byte; // put byte into SPI buffer
    while (!SSPSTATbits.BF) // wait for the transfer to finish
        NOP();
    return SSPBUF; // save the read value
}

uint16_t mcu_power_supply_voltage(float offset, float gain_error, float *mcu_vdd) // MCU power supply voltage
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition time select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_1024_BAND_GAP; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *mcu_vdd = (1.024f * ANALOG_RANGE) / q; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t ctmu_source_current_AN1(float Vref, float offset, float gain_error, float *voltage_AN1) // measures voltage at MCU I/O port after CTMU flowing current
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_01; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_ms(10); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN1 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t measure_voltage_AN1(float Vref, float offset, float gain_error, float *voltage_AN1) // measures voltage at MCU I/O port
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    CTMUCONHbits.IDISSEN = 0; // analog current source output is not grounded
    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_01; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN1 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t ctmu_source_current_AN2(float Vref, float offset, float gain_error, float *voltage_AN2) // measures voltage at MCU I/O port after CTMU flowing current
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_02; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_ms(10); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN2 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t measure_voltage_AN2(float Vref, float offset, float gain_error, float *voltage_AN2) // measures voltage at MCU I/O port
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    CTMUCONHbits.IDISSEN = 0; // analog current source output is not grounded
    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_02; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN2 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t ctmu_source_current_AN9(float Vref, float offset, float gain_error, float *voltage_AN9) // measures voltage at MCU I/O port after CTMU flowing current
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_09; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_ms(10); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN9 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t measure_voltage_AN9(float Vref, float offset, float gain_error, float *voltage_AN9) // measures voltage at MCU I/O port
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    CTMUCONHbits.IDISSEN = 0; // analog current source output is not grounded
    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_09; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN9 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t measure_voltage_AN8(float Vref, float offset, float gain_error, float *voltage_AN8) // measures voltage at MCU I/O port
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    CTMUCONHbits.IDISSEN = 0; // analog current source output is not grounded
    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_08; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN8 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

uint16_t measure_voltage_AN10(float Vref, float offset, float gain_error, float *voltage_AN10) // measures voltage at MCU I/O port
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    CTMUCONHbits.IDISSEN = 0; // analog current source output is not grounded
    ADCON2bits.ACQT = ACQT_20TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_CHANNEL_10; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time o stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_AN10 = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q;
}

void mcu_count_analog_quantums(float offset, float gain_error)
{
    uint16_t timeval = 10; // minaml delay time (nanoseconds)
    uint8_t timestep = 10; // delay time increment for loop cycle (nanoseconds)

    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits

    for (int i = 0; i < 4096; i++) {
        TMR0H = (uint8_t) ((0xFFFF - timeval) >> 8); // 0xFFFF - maximum time in 16-bit timer registers
        TMR0L = (uint8_t) ((0xFFFF - timeval) & 0x00FF); //
        TMR0IF = 0;

        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1;
        TMR0ON = 1; // timer run
        while (!TMR0IF);
        CTMUCONLbits.EDG1STAT = 0;
        TMR0ON = 0; // timer stop  

        ADCON0bits.GODONE = 1; // begin A/D conversation
        while (ADCON0bits.GODONE); // wait for A/D convert complete
        timeval = timeval + timestep;
    }
}

uint16_t internal_capacitor_full_charge(float Vref, float offset, float gain_error, float *full_charge_voltage)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_ms(200); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU        

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *full_charge_voltage = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

uint16_t internal_capacitor_charge_10us(float Vref, float offset, float gain_error, float *voltage_10us)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_us(10); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU        

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_10us = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

uint16_t internal_capacitor_superfluous_voltage(float Vref, float offset, float gain_error, float *superfluous_voltage)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        //__delay_us(10); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU        

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *superfluous_voltage = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

uint16_t internal_capacitor_charge_110us(float Vref, float offset, float gain_error, float *voltage_110us)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum  

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_us(110); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU        

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *voltage_110us = (q * Vref) / ANALOG_RANGE; // // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

uint16_t internal_capacitor_discharge(float Vref, float offset, float gain_error, float *discharge_voltage)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum 

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    *discharge_voltage = (q * Vref) / ANALOG_RANGE; // Vref = (Vin * ADrange)/ADvalue, Vin = (ADvalue * Vref)/ADrange

    return (uint16_t) q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

float mcu_internal_capacitor_capacitance(float Vref, float offset, float gain_error, float current)
{
    uint8_t m = 10; // measurements attempts
    uint16_t a = 0; // place for quantum from A/D
    float q = 0; // accumulated quantum

    ADCON2bits.ACQT = ACQT_0TAD; // A/D acquisition Time Select bits
    ADCON1bits.VCFG = VCFG_VREF_AVDD; // A/D VREF+ configuration bits
    ADCON0bits.CHS = CHS_MUX_DISCONNECT; // A/D analog channel select bits
    for (int i = 0; i < m; i++) {
        CTMUCONHbits.IDISSEN = 1; // drain charge on the circuit
        __delay_us(10); // wait time
        CTMUCONHbits.IDISSEN = 0; // end drain of circuit

        CTMUCONLbits.EDG1STAT = 1; // start flow current from CTMU
        __delay_us(100); // wait time
        CTMUCONLbits.EDG1STAT = 0; // stop flow current from CTMU

        ADCON0bits.GODONE = 1; // start A/D conversion
        while (ADCON0bits.GODONE) // wait for A/D conversion complete
            NOP();
        a = ((uint16_t) (ADRESH << 8) | (ADRESL)); // get the value from the A/D
        q += ((float) a * gain_error) + offset; // save value in A/D quantum
        __delay_us(100); // time to stabilize MCU analog circuits
    }

    q = roundf(q / m); // average value
    q = (q * Vref) / ANALOG_RANGE;

    return (current * 100) / q; // C = (I * T)/V, T = (C/I) * V,  I = (C * V)/T, C/I = T/V
}

void mcu_setup_analog_systems()
{
    ADCON1bits.TRIGSEL = 0b01; // 0b01 - selects the special trigger from the CTMU
    ADCON1bits.VCFG = 0b00; // 0b00 - external VREF+ is AVDD, 0b10 - internal VREF+ 2.0V
    ADCON1bits.VNCFG = 0b00; // A/D VREF- configuration bit (0 - AVSS)
    ADCON0bits.CHS = 0b11100; // analog channel select bits (0b11100 - MUX Disconnect)
    ADCON1bits.CHSN = 0b000; // analog negative channel select bits (Channel 00 (AVSS))
    ADCON2bits.ACQT = 0b010; // A/D acquisition time select bits (0b000 - 0 TAD, 0b001 - 2 TAD, 0b010 - 4 TAD, 0b011 - 6 TAD, 0b100 - 8 TAD)
    ADCON2bits.ADCS = 0b010; // A/D conversion clock select bits (0b110 - FOSC/64, 0b010 - FOSC/32, 0b101 - FOSC/16, 0b001 - FOSC/8, 0b100 - FOSC/4, 0b000 - FOSC/2)
    ADCON2bits.ADFM = 1; // A/D result format select bit (1 - right justified, 0 - left justified)
    ADCON0bits.ADON = 1; // A/D Converter is operating

    CTMUCONHbits.CTTRIG = 0; // CTMU Special Event Trigger bit (0 - CTMU Special Event Trigger is disabled)    
    CTMUCONHbits.CTMUSIDL = 1; // 1 - Stop in Idle Mode bit, 0 - Continues module operation in Idle mode
    CTMUCONHbits.TGEN = 0; // Time Generation Enable bit (0 - Disables edge delay generation)
    CTMUCONHbits.EDGEN = 0; // Edge Enable bit (0 - Edges are blocked)
    CTMUCONHbits.IDISSEN = 0; // Analog Current Source Control bit (0 - Analog current source output is not grounded)
    CTMUCONLbits.EDG1POL = 1; // Edge 1 Polarity Select bit (1 - Edge 1 is programmed for a positive edge response)
    CTMUCONLbits.EDG1SEL = 0b11; // Edge 1 Source Select bits (CTED1 pin)
    CTMUCONLbits.EDG2POL = 1; // Edge 2 Polarity Select bit (1 - Edge 2 is programmed for a positive edge response)
    CTMUCONLbits.EDG2SEL = 0b10; // Edge 2 Source Select bits (CTED2 pin)
    CTMUCONLbits.EDG1STAT = 0; // Edge 1 Status bit (Edge 1 event has occurred)
    CTMUCONLbits.EDG2STAT = 0; // Edge 2 Status bit (Edge 2 event has occurred)
    CTMUICONbits.ITRIM = 0b000000; // Nominal current output specified by IRNG
    CTMUICONbits.IRNG = 0b01; // Current Source Range Select bits (0b01 - 0.55uA, 0b10 - 5.5uA, 0b11 - 55uA)
    CTMUCONHbits.CTMUEN = 1; // CTMU Enable bit

    CVRCONbits.CVROE = 0; // 0 - CVREF voltage level is disconnected from CVREF pin, 1 - CVREF voltage level is output on CVREF pin
    CVRCONbits.CVRSS = 0; // 0 - Comparator reference source, CVRSRC = AVDD ? AVSS, 1 - Comparator reference source, CVRSRC = VREF+ ? VREF-
    CVRCONbits.CVR = 0b00000; // CVREF = (VREF-) + (CVR<4:0>/32) ? (VREF+ ? VREF-), CVREF = (AVSS) + (CVR<4:0>/32) ? (AVDD ? AVSS)
    CVRCONbits.CVREN = 0; // 0 - CVREF circuit powered down, 1 - CVREF circuit powered on

    CM1CONbits.COE = 0; // 1 - Comparator output is present on the CxOUT pin, 0 - Comparator output is internal only
    CM1CONbits.CPOL = 0; // 1 - Comparator output is inverted, 0 - Comparator output is not inverted
    CM1CONbits.EVPOL = 0b00; // 00 - Interrupt generation is disabled
    CM1CONbits.CREF = 0b00; // 1 - Non-inverting input connects to internal CVREF voltage, 0 - Non-inverting input connects to CxINA pin
    CM1CONbits.CCH = 0b00; // 00 - Inverting input of comparator connects to C1INB pin
    CM1CONbits.CON = 0; // Comparator Enable bit

    CM2CONbits.COE = 0; // 1 - Comparator output is present on the CxOUT pin, 0 - Comparator output is internal only
    CM2CONbits.CPOL = 0; // 1 - Comparator output is inverted, 0 - Comparator output is not inverted
    CM2CONbits.EVPOL = 0b00; // 00 - Interrupt generation is disabled
    CM2CONbits.CREF = 0b00; // 1 - Non-inverting input connects to internal CVREF voltage, 0 - Non-inverting input connects to CxINA pin
    CM2CONbits.CCH = 0b00; // 00 - Inverting input of comparator connects to C1INB pin
    CM2CONbits.CON = 0; // Comparator Enable bit
}
