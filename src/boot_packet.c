/**
 * \file            boot_core.c
 * \brief           Functions of working with packets.
 * \copyright       DC Vostok Vladivostok 2023
 */

#include "boot_packet.h"


static volatile struct
{
    uint8_t mem[PACKET_FIFO_BYTES];
    uint32_t wr_ptr;
    uint32_t rd_ptr;
    uint32_t full;
    uint32_t empty;
} packet_fifo;


/**
 * \brief           FIFO writing function called from the UART_RX interrupt
 * 
 * \param[in]       data: Byte of data 
 */
static inline __attribute__((always_inline)) void packet_fifo_write(uint8_t data)
{
    if (!packet_fifo.full) {
        packet_fifo.mem[packet_fifo.wr_ptr] = data;
        packet_fifo.empty = 0;
        if (packet_fifo.wr_ptr == (PACKET_FIFO_BYTES - 1))
            packet_fifo.wr_ptr = 0;
        else
            packet_fifo.wr_ptr++;
    }

    if (packet_fifo.wr_ptr == packet_fifo.rd_ptr)
        packet_fifo.full = 1;
    else
        packet_fifo.full = 0;
}


void packet_fifo_init()
{
    packet_fifo.wr_ptr = 0;
    packet_fifo.rd_ptr = 0;
    packet_fifo.full = 0;
    packet_fifo.empty = 1;
}

uint8_t packet_fifo_read()
{
    uint8_t data = 0;

    while (packet_fifo.empty) {
    };

    data = packet_fifo.mem[packet_fifo.rd_ptr];
    if (packet_fifo.rd_ptr == (PACKET_FIFO_BYTES - 1))
        packet_fifo.rd_ptr = 0;
    else
        packet_fifo.rd_ptr++;

    if ((packet_fifo.wr_ptr == packet_fifo.rd_ptr) && !packet_fifo.full)
        packet_fifo.empty = 1;

    return data;
}

uint16_t crc_upd(uint16_t crc_in, uint8_t data)
{
    uint32_t crc = crc_in;
    uint32_t in = data | 0x100;

    do {
        crc <<= 1;
        in <<= 1;
        if (in & 0x100)
            ++crc;
        if (crc & 0x10000)
            crc ^= 0x1021;
    } while (!(in & 0x10000));

    return crc & 0xffffu;
}

void packet_receive(Packet_TypeDef* rx_packet)
{
    uint16_t rx_signature;
    uint8_t rx_cmd;
    uint8_t rx_cmd_inv;
    uint16_t rx_data_n;
    uint16_t pre_crc;

    //Search for a signature
    rx_signature = 0x0000;
    while (rx_signature != PACKET_HOST_SIGN) {
        rx_signature = (rx_signature >> 8) | (uint16_t)(packet_fifo_read() << 8);
    }
    //Read service information
    rx_cmd = packet_fifo_read();
    rx_cmd_inv = packet_fifo_read();
    rx_data_n = packet_fifo_read_u16();

    //checking the correctness of the command
    if ((rx_cmd ^ rx_cmd_inv) != 0xFF) {
        rx_packet->cmd_code = CMD_NONE;
        rx_packet->tmp_data8[0] = MSG_ERR_CMD;
    }
    //if everything is correct, then we start counting crc
    else {
        pre_crc = 0;
        pre_crc = crc_upd(pre_crc, rx_cmd);
        pre_crc = crc_upd(pre_crc, rx_cmd_inv);
        pre_crc = crc_upd_u16(pre_crc, rx_data_n);
        //pass what we have parsed to the kernel, the crc will be counted and checked already there in the handler of a specific command
        rx_packet->cmd_code = rx_cmd;
        rx_packet->data_n = rx_data_n;
        rx_packet->crc = pre_crc;
    }
}

uint32_t packet_transmit_status_busy()
{
    return UART->FR_bit.BUSY | !UART->FR_bit.TXFE;
}

void packet_transmit(Packet_TypeDef* tx_packet)
{
    uint16_t crc = 0;

    while (packet_transmit_status_busy()) {
    };
    DBG_PRINT(0x04);
    DBG_PRINT(tx_packet->cmd_code);

    UART->DR = PACKET_DEVICE_SIGN & 0x00FF;
    UART->DR = (PACKET_DEVICE_SIGN & 0xFF00) >> 8;

    UART->DR = tx_packet->cmd_code;
    crc = crc_upd(crc, tx_packet->cmd_code);

    UART->DR = ~tx_packet->cmd_code;
    crc = crc_upd(crc, ~tx_packet->cmd_code);

    UART->DR = tx_packet->data_n & 0x00FF;
    UART->DR = (tx_packet->data_n & 0xFF00) >> 8;
    crc = crc_upd_u16(crc, tx_packet->data_n);

    for (uint16_t i = 0; i < tx_packet->data_n; i++) {
        while (!UART->RIS_bit.TXRIS || UART->FR_bit.TXFF) {
        };
        UART->DR = tx_packet->tmp_data8[i];
        UART->ICR = UART_ICR_TXIC_Msk;
        crc = crc_upd(crc, tx_packet->tmp_data8[i]);
    }

    UART->DR = crc & 0x00FF;
    UART->DR = (crc & 0xFF00) >> 8;
}

uint32_t packet_fifo_read_u32()
{
    uint32_t data = 0;

    data = (uint32_t)(packet_fifo_read() << 0);
    data |= (uint32_t)(packet_fifo_read() << 8);
    data |= (uint32_t)(packet_fifo_read() << 16);
    data |= (uint32_t)(packet_fifo_read() << 24);

    return data;
}


uint16_t packet_fifo_read_u16()
{
    uint16_t data = 0;

    data = (uint16_t)(packet_fifo_read() << 0);
    data |= (uint16_t)(packet_fifo_read() << 8);

    return data;
}


uint16_t crc_upd_u32(uint16_t crc_in, uint32_t data)
{
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0x000000FF) >> 0));
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0x0000FF00) >> 8));
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0x00FF0000) >> 16));
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0xFF000000) >> 24));

    return crc_in;
}

uint16_t crc_upd_u16(uint16_t crc_in, uint16_t data)
{
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0x00FF) >> 0));
    crc_in = crc_upd(crc_in, (uint8_t)((data & 0xFF00) >> 8));

    return crc_in;
}


RAMFUNC void UART_RX_IRQHandler()
{
    UART->ICR = UART_ICR_RXIC_Msk;

    while (!UART->FR_bit.RXFE) {
        packet_fifo_write(UART->DR_bit.DATA);
    }
}
