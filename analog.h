#ifndef CTMU_H
#define	CTMU_H

#define ANALOG_RANGE 4095.0f

#define MCP4921_CS LATBbits.LATB4

#define mcp_select()   MCP4921_CS = 0
#define mcp_unselect() MCP4921_CS = 1

extern float internal_capacitance;
extern float offset, gain_error, offset_voltage;
extern float ctmu_current_0_55_uA, ctmu_current_5_5_uA, ctmu_current_55_uA;
extern float mcu_vdd, full_charge_voltage, discharge_voltage;

/* MCP4921 DAC */
#define MCP4921_DAC_SELECT  0x8000
#define MCP4921_VREF_BUFFER 0x4000
#define MCP4921_GAIN_SELECT 0x2000
#define MCP4921_SHDN_OUTPUT 0x1000

/* A/D channel select bits */
#define CHS_CHANNEL_00 0b00000
#define CHS_CHANNEL_01 0b00001
#define CHS_CHANNEL_02 0b00010
#define CHS_CHANNEL_03 0b00011
#define CHS_CHANNEL_04 0b00101
#define CHS_CHANNEL_05 0b00101
#define CHS_CHANNEL_06 0b00110
#define CHS_CHANNEL_07 0b00111
#define CHS_CHANNEL_08 0b01000
#define CHS_CHANNEL_09 0b01001
#define CHS_CHANNEL_10 0b01010

#define CHS_RESERVED_00 0b01010
#define CHS_RESERVED_01 0b01100
#define CHS_RESERVED_02 0b01101
#define CHS_RESERVED_03 0b01110
#define CHS_RESERVED_04 0b01111
#define CHS_RESERVED_05 0b10000
#define CHS_RESERVED_06 0b10001
#define CHS_RESERVED_07 0b10010
#define CHS_RESERVED_08 0b10011
#define CHS_RESERVED_09 0b10100
#define CHS_RESERVED_10 0b10101
#define CHS_RESERVED_11 0b10110
#define CHS_RESERVED_12 0b10110
#define CHS_RESERVED_13 0b10111
#define CHS_RESERVED_14 0b11000
#define CHS_RESERVED_15 0b11001
#define CHS_RESERVED_16 0b11010
#define CHS_RESERVED_17 0b11011
#define CHS_RESERVED_18 0b11011

#define CHS_MUX_DISCONNECT    0b11100
#define CHS_TEMPERATURE_DIODE 0b11101
#define CHS_VDDCORE           0b11110
#define CHS_1024_BAND_GAP     0b11111

/* A/D special trigger select bits */
#define TRIGSEL_CCP2   0b11
#define TRIGSEL_TIMER1 0b10
#define TRIGSEL_CTMU   0b01
#define TRIGSEL_ECCP1  0b00

/* A/D VREF+ configuration bits */
#define VCFG_INTERNAL_VREF_4_1 0b11 // Internal VREF+ 4.1V
#define VCFG_INTERNAL_VREF_2_0 0b10 // internal VREF+ 2.0V
#define VCFG_EXTERNAL_VREF     0b01 // External VREF+
#define VCFG_VREF_AVDD         0b00 // external VREF+ is AVDD

/* A/D VREF- Configuration bit */
#define VNCFG_EXTERNAL_VREF 1
#define VNCFG_AVSS          0

/* A/D negative channel select bits */
#define CHSN_CHANNEL_07   0b00000
#define CHSN_CHANNEL_06   0b00001
#define CHSN_CHANNEL_05   0b00010
#define CHSN_CHANNEL_04   0b00011
#define CHSN_CHANNEL_03   0b00101
#define CHSN_CHANNEL_02   0b00101
#define CHSN_CHANNEL_00   0b00110
#define CHSN_CHANNEL_AVSS 0b00111

/* A/D result format select bit */
#define ADFM_JUSTIFY_RIGHT 1
#define ADFM_JUSTIFY_LEFT  0

/* A/D acquisition time select bits */
#define ACQT_20TAD 0b111
#define ACQT_16TAD 0b110
#define ACQT_12TAD 0b101
#define ACQT_8TAD  0b100
#define ACQT_6TAD  0b011
#define ACQT_4TAD  0b010
#define ACQT_2TAD  0b001
#define ACQT_0TAD  0b000

/* A/D conversion clock select bits */
#define ADCS_FRC_HIGH 0b111
#define ADCS_FOSC_64  0b110
#define ADCS_FOSC_16  0b101
#define ADCS_FOSC_4   0b100
#define ADCS_FRC_LOW  0b011
#define ADCS_FOSC_32  0b010
#define ADCS_FOSC_8   0b001
#define ADCS_FOSC_2   0b000

/* function prototypes */
uint8_t mcp_spi_send(uint8_t byte);
uint16_t mcu_power_supply_voltage(float offset, float gain_error, float *mcu_vdd);
uint16_t ctmu_source_current_AN1(float Vref, float offset, float gain_error, float *voltage_AN1);
uint16_t measure_voltage_AN1(float Vref, float offset, float gain_error, float *voltage_AN9);
uint16_t ctmu_source_current_AN2(float Vref, float offset, float gain_error, float *voltage_AN2);
uint16_t measure_voltage_AN2(float Vref, float offset, float gain_error, float *voltage_AN9);
uint16_t ctmu_source_current_AN9(float Vref, float offset, float gain_error, float *voltage_AN9);
uint16_t measure_voltage_AN9(float Vref, float offset, float gain_error, float *voltage_AN9);
uint16_t measure_voltage_AN8(float Vref, float offset, float gain_error, float *voltage_AN8);
uint16_t measure_voltage_AN10(float Vref, float offset, float gain_error, float *voltage_AN10);
uint16_t internal_capacitor_full_charge(float Vref, float offset, float gain_error, float *full_charge_voltage);
uint16_t internal_capacitor_charge_10us(float Vref, float offset, float gain_error, float *voltage_10us);
uint16_t internal_capacitor_superfluous_voltage(float Vref, float offset, float gain_error, float *superfluous_voltage);
uint16_t internal_capacitor_charge_110us(float Vref, float offset, float gain_error, float *voltage_110us);
uint16_t internal_capacitor_discharge(float Vref, float offset, float gain_error, float *voltage_discharge);
float mcu_internal_capacitor_capacitance(float Vref, float offset, float gain_error, float current);
void mcu_count_analog_quantums(float offset, float gain_error);
void mcu_setup_analog_systems();

#endif	/* CTMU_H */

