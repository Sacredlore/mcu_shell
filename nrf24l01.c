#include "mcu_shell.h"
#include "nrf24l01.h"

//struct {
//    uint8_t rx_addr_p0[5];
//    uint8_t rx_addr_p1[5];
//    uint8_t rx_addr_p2;
//    uint8_t rx_addr_p3;
//    uint8_t rx_addr_p4;
//    uint8_t rx_addr_p5;
//    uint8_t tx_addr[5];
//} nrf_addr = {
//    { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7},
//    { 0xC2, 0xC2, 0xC2, 0xC2, 0xC2},
//    { 0xC3},
//    { 0xC4},
//    { 0xC5},
//    { 0xC6},
//    { 0xE7, 0xE7, 0xE7, 0xE7, 0xE7}
//};

struct {
    uint8_t rx_addr_p0[5];
    uint8_t rx_addr_p1[5];
    uint8_t rx_addr_p2;
    uint8_t rx_addr_p3;
    uint8_t rx_addr_p4;
    uint8_t rx_addr_p5;
    uint8_t tx_addr[5];
} nrf_addr = {
    { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA},
    { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB},
    { 0xD3},
    { 0xD4},
    { 0xD5},
    { 0xD6},
    { 0x77, 0x77, 0x77, 0x77, 0x77}
};

uint8_t nrf_spi_send(uint8_t byte) /* CKP = 0, CKE = 1, SMP = 1 */
{
    SSPBUF = byte;
    while (!SSPSTATbits.BF)
        NOP();
    return SSPBUF;
}

uint8_t nrf_read_register(uint8_t reg_addr, uint8_t *reg_val)
{
    uint8_t reg_status;

    nrf_select();
    reg_status = nrf_spi_send(NRF_R_REGISTER | reg_addr);
    *reg_val = nrf_spi_send(0x00); // send something
    nrf_unselect();

    return reg_status;
}

uint8_t nrf_write_register(uint8_t reg_addr, uint8_t reg_val)
{
    uint8_t reg_status;

    nrf_select();
    reg_status = nrf_spi_send(NRF_W_REGISTER | reg_addr);
    nrf_spi_send(reg_val);
    nrf_unselect();

    return reg_status;
}

void nrf_flush_tx_fifo(void)
{
    nrf_select();
    nrf_spi_send(NRF_FLUSH_TX);
    nrf_select();
}

void nrf_flush_rx_fifo(void)
{
    nrf_select();
    nrf_spi_send(NRF_FLUSH_RX);
    nrf_unselect();
}

void nrf_write_payload(uint8_t *payload, uint8_t width)
{
    nrf_select();
    nrf_spi_send(NRF_W_TX_PAYLOAD);
    while (width--)
        nrf_spi_send(*payload++);
    nrf_unselect();
}

void nrf_read_payload(uint8_t *payload, uint8_t width)
{
    nrf_select();
    nrf_spi_send(NRF_R_RX_PAYLOAD);
    while (width--)
        *payload++ = nrf_spi_send(0x00); // send something
    nrf_unselect();
}

void nrf_power_down(void)
{
    uint8_t reg_config, reg_status;

    nrf_ce_low(); // standby mode
    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    CLEAR_BITMASK(reg_config, NRF_CONFIG_PWR_UP);
    nrf_write_register(NRF_CONFIG, reg_config); // write configuration register
}

void nrf_power_up(bool disable_aa)
{
    uint8_t reg_config, reg_status;

    nrf_ce_low(); // standby mode
    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    SET_BITMASK(reg_config, NRF_CONFIG_PWR_UP);
    nrf_write_register(NRF_CONFIG, reg_config); // write configuration register
    __delay_us(150);

    nrf_write_register(NRF_RX_PW_P0, NRF_PAYLOAD_WIDTH); // set payload width
    nrf_write_register(NRF_RX_PW_P1, NRF_PAYLOAD_WIDTH); //
    nrf_write_register(NRF_RX_PW_P2, NRF_PAYLOAD_WIDTH); //
    nrf_write_register(NRF_RX_PW_P3, NRF_PAYLOAD_WIDTH); //
    nrf_write_register(NRF_RX_PW_P4, NRF_PAYLOAD_WIDTH); //
    nrf_write_register(NRF_RX_PW_P5, NRF_PAYLOAD_WIDTH); //
    nrf_write_register(NRF_SETUP_RETR, NRF_SETUP_RETR_ARD_1000 | NRF_SETUP_RETR_ARC_3);
    nrf_write_register(NRF_RF_SETUP, NRF_RF_SETUP_RF_DR | NRF_RF_SETUP_RF_PWR_0);

    if (disable_aa == true) {
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_NONE);
    } else {
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_ALL);
    }

    nrf_flush_tx_fifo();
    nrf_flush_rx_fifo(); // clean all FIFOs
}

void nrf_disable_aa(void) // disable auto acknowledgment
{
    uint8_t reg_config, reg_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    if (PRIM_RX_IS_SET(reg_config)) {
        nrf_ce_low(); // standby mode
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_NONE);
        nrf_ce_high();
        __delay_us(130); // standby mode to receiver switch-over delay
    } else {
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_NONE);
    }
}

void nrf_enable_aa(void) // enable auto acknowledgment
{
    uint8_t reg_config, reg_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    if (PRIM_RX_IS_SET(reg_config)) {
        nrf_ce_low(); // standby mode
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_ALL);
        nrf_ce_high();
        __delay_us(130); // standby mode to receiver switch-over delay
    } else {
        nrf_write_register(NRF_EN_AA, NRF_EN_AA_ENAA_ALL);
    }
}

void nrf_set_as_transmitter(void)
{
    uint8_t reg_config, reg_status;

    nrf_ce_low(); // standby mode
    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    CLEAR_BITMASK(reg_config, NRF_CONFIG_PRIM_RX);
    nrf_write_register(NRF_CONFIG, reg_config);
    __delay_us(130); // standby mode to transmitter switch-over delay
}

void nrf_set_as_receiver(void)
{
    uint8_t reg_config, reg_status;

    nrf_ce_low(); // standby mode
    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    SET_BITMASK(reg_config, NRF_CONFIG_PRIM_RX);
    nrf_write_register(NRF_CONFIG, reg_config);
    nrf_ce_high(); // set high in receiver mode
    __delay_us(130); // standby mode to receiver switch-over delay
}

void nrf_mask_interrupts(void)
{
    uint8_t reg_config, reg_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    SET_BITMASK(reg_config, NRF_CONFIG_MASK_RX_DR | NRF_CONFIG_MASK_TX_DS | NRF_CONFIG_MASK_MAX_RT); // mask interrupts
    SET_BITMASK(reg_status, NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT); // clear interrupts    
    if (PRIM_RX_IS_SET(reg_config)) {
        nrf_ce_low(); // standby mode
        nrf_write_register(NRF_CONFIG, reg_config);
        nrf_write_register(NRF_STATUS, reg_status);
        nrf_ce_high();
        __delay_us(130); // standby mode to receiver switch-over delay
    } else {
        nrf_write_register(NRF_CONFIG, reg_config);
        nrf_write_register(NRF_STATUS, reg_status);
    }
}

void nrf_unmask_interrupts(void)
{
    uint8_t reg_config, reg_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    CLEAR_BITMASK(reg_config, NRF_CONFIG_MASK_RX_DR | NRF_CONFIG_MASK_TX_DS | NRF_CONFIG_MASK_MAX_RT); // unmask interrupts
    if (PRIM_RX_IS_SET(reg_config)) {
        nrf_ce_low(); // standby mode
        nrf_write_register(NRF_CONFIG, reg_config);
        nrf_ce_high();
        __delay_us(130); // standby mode to receiver switch-over delay
    } else {
        nrf_write_register(NRF_CONFIG, reg_config);
    }
}

void nrf_clear_all_interrupts(void)
{
    uint8_t reg_status;

    nrf_read_register(NRF_STATUS, &reg_status);
    SET_BITMASK(reg_status, NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);
    nrf_write_register(NRF_STATUS, reg_status);
}

void nrf_interrupt(void) // TRISB0 = 1, INT0IE = 1, INTEDG1 = 0
{
    uint8_t reg_config, reg_status, reg_fifo_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    reg_status = nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
    if (RX_DR_IS_SET(reg_status)) { // new data in RX FIFO

    }
    if (TX_DS_IS_SET(reg_status)) { // packet sent on TX 

    }
    if (MAX_RT_IS_SET(reg_status)) { // maximum number of TX retries
        nrf_flush_tx_fifo();
        nrf_flush_rx_fifo();
    }
    nrf_write_register(NRF_STATUS, reg_status);
}

//void nrf_transmit_sync(void)
//{
//    char string_print[64];
//    uint8_t payload[NRF_PAYLOAD_WIDTH];
//    static uint8_t sync_id = 0;
//
//    if (tx_io_nrf.cnt <= NRF_PAYLOADS_COUNT) { // save in local buffer if there is free space
//        memset(payload, 0, NRF_PAYLOAD_WIDTH);
//        payload[0] = NRF_PAYLOAD_SYNC;
//        payload[1] = sync_id++;
//        for (int i = 0; i < NRF_PAYLOAD_WIDTH; i++) {
//            tx_io_nrf.mem[tx_io_nrf.pri][i] = payload[i];
//        }
//        tx_io_nrf.cnt++;
//        tx_io_nrf.pri++;
//        if (tx_io_nrf.pri >= NRF_PAYLOADS_COUNT)
//            tx_io_nrf.pri = 0;
//    } else { // no free space in local buffer
//        sprintf(string_print, "no free space in local buffer\r\n");
//        uart_transmit(string_print, strlen(string_print));
//    }
//}

//void nrf_receive_sync(void)
//{
//    char string_print[64];
//    uint8_t payload[NRF_PAYLOAD_WIDTH];
//
//    if (rx_io_nrf.cnt > 0) { //there is something to receive from local buffer
//        memset(payload, 0, NRF_PAYLOAD_WIDTH);
//        for (int i = 0; i < NRF_PAYLOAD_WIDTH; i++) {
//            payload[i] = rx_io_nrf.mem[rx_io_nrf.pri][i];
//        }
//        rx_io_nrf.cnt--;
//        rx_io_nrf.pri++;
//        if (rx_io_nrf.pri >= NRF_PAYLOADS_COUNT)
//            rx_io_nrf.pri = 0;
//
//        print_sync(payload);
//    } else { // nothing to receive from local buffer
//        sprintf(string_print, "nothing to receive from local buffer\r\n");
//        uart_transmit(string_print, strlen(string_print));
//    }
//}

//void nrf_module_as_receiver(void) // demo function
//{
//    char string_print[64];
//    uint8_t payload[NRF_PAYLOAD_WIDTH] = {'N', 'R', 'F', '2', '4', 'L', '0', '1'};
//    static uint8_t ping_id = 0;
//    uint8_t reg_status, reg_fifo_status, reg_cd;
//
//    nrf_power_up();
//    nrf_set_as_receiver();
//
//    for (;;) {
//        for (int i = 0; i < 25; i++) { // receive loop
//            reg_status = nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
//            if (RX_FIFO_NOT_EMPTY(reg_fifo_status)) { // there is something to receive from wireless device
//                if (rx_io_nrf.cnt <= NRF_PAYLOADS_COUNT) { // save payload in local buffer if there is free space
//                    nrf_read_payload(rx_io_nrf.mem[rx_io_nrf.sec], NRF_PAYLOAD_WIDTH);
//                    rx_io_nrf.cnt++;
//                    rx_io_nrf.sec++;
//                    if (rx_io_nrf.sec >= NRF_PAYLOADS_COUNT)
//                        rx_io_nrf.sec = 0;
//                } else { // no free space in local buffer
//                    sprintf(string_print, "no free space in local buffer\r\n");
//                    uart_transmit(string_print, strlen(string_print));
//                }
//            } else { // nothing to receive from wireless device
//                sprintf(string_print, "nothing to receive from wireless device\r\n");
//                uart_transmit(string_print, strlen(string_print));
//            }
//        }
//
//        nrf_receive_sync(); // load payload from local buffer
//    }
//}

//void nrf_module_as_transmitter() // demo function
//{
//    char string_print[64];
//    uint8_t payload[NRF_PAYLOAD_WIDTH] = {'N', 'R', 'F', '2', '4', 'L', '0', '1'};
//    static uint8_t ping_id = 0;
//    uint8_t reg_status, reg_fifo_status, reg_observe_tx;
//
//    nrf_power_up();
//    nrf_set_as_transmitter();
//
//    for (;;) {
//        nrf_transmit_sync(); // save packet in local buffer
//
//        for (int i = 0; i < 25; i++) { // transmission loop
//            reg_status = nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
//            if ((TX_FIFO_EMPTY(reg_fifo_status) && TX_FIFO_NOT_FULL(reg_fifo_status)) // there is free space in wireless device FIFO
//                    || (TX_FIFO_NOT_EMPTY(reg_fifo_status) && TX_FIFO_NOT_FULL(reg_fifo_status))) {
//                if (tx_io_nrf.cnt > 0) { // there is something to transmit from local buffer
//                    nrf_write_payload(tx_io_nrf.mem[tx_io_nrf.sec], NRF_PAYLOAD_WIDTH);
//                    tx_io_nrf.cnt--;
//                    tx_io_nrf.sec++;
//                    if (tx_io_nrf.sec >= NRF_PAYLOADS_COUNT)
//                        tx_io_nrf.sec = 0;
//                } else { // nothing to transmit from local buffer
//                    sprintf(string_print, "nothing to transmit from local buffer\r\n");
//                    uart_transmit(string_print, strlen(string_print));
//                }
//            }
//
//            reg_status = nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
//            if ((TX_FIFO_NOT_EMPTY(reg_fifo_status) && TX_FIFO_NOT_FULL(reg_fifo_status)) // there is something to transmit in wireless device FIFO
//                    || (TX_FIFO_NOT_EMPTY(reg_fifo_status) && TX_FIFO_FULL(reg_fifo_status))) {
//
//                nrf_ce_high(), __delay_us(10), nrf_ce_low(); // transition triggers transmission
//                reg_status = nrf_read_register(NRF_OBSERVE_TX, &reg_observe_tx);
//                if (TX_DS_IS_SET(reg_status)) {
//                    nrf_ce_low();
//                    nrf_write_register(NRF_STATUS, reg_status);
//                }
//                if (MAX_RT_IS_SET(reg_status)) {
//                    nrf_ce_low();
//                    nrf_write_register(NRF_STATUS, reg_status);
//                }
//            }
//        }
//    }
//}

void nrf_override_pipes_address(void)
{
    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P0);
    nrf_spi_send(nrf_addr.rx_addr_p0[0]);
    nrf_spi_send(nrf_addr.rx_addr_p0[1]);
    nrf_spi_send(nrf_addr.rx_addr_p0[2]);
    nrf_spi_send(nrf_addr.rx_addr_p0[3]);
    nrf_spi_send(nrf_addr.rx_addr_p0[4]);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P1);
    nrf_spi_send(nrf_addr.rx_addr_p1[0]);
    nrf_spi_send(nrf_addr.rx_addr_p1[1]);
    nrf_spi_send(nrf_addr.rx_addr_p1[2]);
    nrf_spi_send(nrf_addr.rx_addr_p1[3]);
    nrf_spi_send(nrf_addr.rx_addr_p1[4]);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P2);
    nrf_spi_send(nrf_addr.rx_addr_p2);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P2);
    nrf_spi_send(nrf_addr.rx_addr_p3);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P2);
    nrf_spi_send(nrf_addr.rx_addr_p4);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_RX_ADDR_P2);
    nrf_spi_send(nrf_addr.rx_addr_p5);
    nrf_unselect();

    nrf_select();
    nrf_spi_send(NRF_W_REGISTER | NRF_TX_ADDR);
    nrf_spi_send(nrf_addr.tx_addr[0]);
    nrf_spi_send(nrf_addr.tx_addr[1]);
    nrf_spi_send(nrf_addr.tx_addr[2]);
    nrf_spi_send(nrf_addr.tx_addr[3]);
    nrf_spi_send(nrf_addr.tx_addr[4]);
    nrf_unselect();
}

void nrf_print_pipes_address(void)
{
    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P0);
    nrf_addr.rx_addr_p0[0] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p0[1] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p0[2] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p0[3] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p0[4] = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p0[0]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p0[1]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p0[2]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p0[3]);
    uart_print_format("0x%02x\r\n", nrf_addr.rx_addr_p0[4]);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P1);
    nrf_addr.rx_addr_p1[0] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p1[1] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p1[2] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p1[3] = nrf_spi_send(0x00);
    nrf_addr.rx_addr_p1[4] = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p1[0]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p1[1]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p1[2]);
    uart_print_format("0x%02x, ", nrf_addr.rx_addr_p1[3]);
    uart_print_format("0x%02x\r\n", nrf_addr.rx_addr_p1[4]);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P2);
    nrf_addr.rx_addr_p2 = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02\r\n", nrf_addr.rx_addr_p2);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P2);
    nrf_addr.rx_addr_p3 = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x\r\n", nrf_addr.rx_addr_p3);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P2);
    nrf_addr.rx_addr_p4 = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x\r\n", nrf_addr.rx_addr_p4);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_RX_ADDR_P2);
    nrf_addr.rx_addr_p5 = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x\r\n", nrf_addr.rx_addr_p5);

    nrf_select();
    nrf_spi_send(NRF_R_REGISTER | NRF_TX_ADDR);
    nrf_addr.tx_addr[0] = nrf_spi_send(0x00);
    nrf_addr.tx_addr[1] = nrf_spi_send(0x00);
    nrf_addr.tx_addr[2] = nrf_spi_send(0x00);
    nrf_addr.tx_addr[3] = nrf_spi_send(0x00);
    nrf_addr.tx_addr[4] = nrf_spi_send(0x00);
    nrf_unselect();

    uart_print_format("0x%02x, ", nrf_addr.tx_addr[0]);
    uart_print_format("0x%02x, ", nrf_addr.tx_addr[1]);
    uart_print_format("0x%02x, ", nrf_addr.tx_addr[2]);
    uart_print_format("0x%02x, ", nrf_addr.tx_addr[3]);
    uart_print_format("0x%02x\r\n", nrf_addr.tx_addr[4]);
}

void nrf_print_registers(void)
{
    uint8_t reg_config, reg_status, reg_fifo_status;
    uint8_t reg_observe_tx, reg_cd;

    nrf_read_register(NRF_CONFIG, &reg_config);
    nrf_read_register(NRF_STATUS, &reg_status);
    nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
    nrf_read_register(NRF_OBSERVE_TX, &reg_observe_tx);
    nrf_read_register(NRF_CD, &reg_cd);

    uart_print_format("CONFIG:      0x%02x\r\n", reg_config);
    uart_print_format("STATUS:      0x%02x\r\n", reg_status);
    uart_print_format("FIFO_STATUS: 0x%02x\r\n", reg_fifo_status);
    uart_print_format("OBSERVE_TX:  0x%02x\r\n", reg_observe_tx);
    uart_print_format("CD:          0x%02x\r\n", reg_cd);
}

void nrf_print_status_register(uint8_t reg_status)
{
    if (RX_DR_IS_SET(reg_status)) {
        uart_print_format("RX_DR:1\r\n");
    } else {
        uart_print_format("RX_DR:0\r\n");
    }
    if (TX_DS_IS_SET(reg_status)) {
        uart_print_format("TX_DS:1\r\n");
    } else {
        uart_print_format("TX_DS:0\r\n");
    }
    if (MAX_RT_IS_SET(reg_status)) {
        uart_print_format("MAX_RT:1\r\n");
    } else {
        uart_print_format("MAX_RT:0\r\n");
    }
}

void nrf_print_config_register(void)
{
    uint8_t reg_config, reg_status;

    reg_status = nrf_read_register(NRF_CONFIG, &reg_config);
    if (PRIM_RX_IS_SET(reg_config)) {
        uart_print_format("PRIM_RX:1\r\n");
    } else {
        uart_print_format("PRIM_RX:0\r\n");
    }
    if (POWER_UP_IS_SET(reg_config)) {
        uart_print_format("POWER_UP:1\r\n");
    } else {
        uart_print_format("POWER_UP:0\r\n");
    }
    if (MASK_RX_DR_IS_SET(reg_config)) {
        uart_print_format("MASK_RX_DR:1\r\n");
    } else {
        uart_print_format("MASK_RX_DR:0\r\n");
    }
    if (MASK_TX_DS_IS_SET(reg_config)) {
        uart_print_format("MASK_TX_DS:1\r\n");
    } else {
        uart_print_format("MASK_TX_DS:0\r\n");
    }
    if (MASK_MAX_RT_IS_SET(reg_config)) {
        uart_print_format("MASK_MAX_RT:1\r\n");
    } else {
        uart_print_format("MASK_MAX_RT:0\r\n");
    }
    nrf_print_status_register(reg_status);
}


void nrf_print_fifo_status_register(void)
{
    uint8_t reg_status, reg_fifo_status;
    
    reg_status = nrf_read_register(NRF_FIFO_STATUS, &reg_fifo_status);
    if (TX_FULL(reg_fifo_status)) {
        uart_print_format("TX_FULL:1\r\n");
    } else {
        uart_print_format("TX_FULL:0\r\n");
    }
    if (TX_EMPTY(reg_fifo_status)) {
        uart_print_format("TX_EMPTY:1\r\n");
    } else {
        uart_print_format("TX_EMPTY:0\r\n");
    }
    if (RX_FULL(reg_fifo_status)) {
        uart_print_format("RX_FULL:1\r\n");
    } else {
        uart_print_format("RX_FULL:0\r\n");
    }
    if (RX_EMPTY(reg_fifo_status)) {
        uart_print_format("RX_EMPTY:1\r\n");
    } else {
        uart_print_format("RX_EMPTY:0\r\n");
    }    
    
    nrf_print_status_register(reg_status);
}

void nrf_print_cd_register(void)
{
    uint8_t reg_status, reg_cd;

    reg_status = nrf_read_register(NRF_CD, &reg_cd);
    if (CD_IS_SET(reg_cd)) {
        uart_print_format("CD:1\r\n");
    } else {
        uart_print_format("CD:0\r\n");
    }
    nrf_print_status_register(reg_status);
}
