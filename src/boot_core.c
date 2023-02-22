/**
 * \file            boot_core.c
 * \brief           Bootloader kernel. Processing of received commands and formation of outgoing packets.
 * \author          DC Vostok
 * \copyright       DC Vostok Vladivostok 2023
 */

#include "boot_core.h"
#include "boot_flash.h"
#include "boot_packet.h"
#include <string.h>

//-- Private function prototypes -----------------------------------------------
static RAMFUNC void msg_cmd(Packet_TypeDef* packet);
static RAMFUNC void get_info_cmd(Packet_TypeDef* packet);
static RAMFUNC void get_cfgword_cmd(Packet_TypeDef* packet);
static RAMFUNC void set_cfgword_cmd(Packet_TypeDef* packet);
static RAMFUNC void read_page_cmd(Packet_TypeDef* packet);
static RAMFUNC void write_page_cmd(Packet_TypeDef* packet);
static RAMFUNC void erase_cmd(Packet_TypeDef* packet);
static RAMFUNC void exit_cmd(Packet_TypeDef* packet);

int wait_uart_rx(uint32_t value){
    uint32_t timeout_start;
    BIT_BAND_PER(TIMEOUT_TMR->CTRL, TMR_CTRL_ON_Msk) = 0;
    TIMEOUT_TMR->LOAD = 0xffffffffu;
    TIMEOUT_TMR->VALUE = TIMEOUT_TMR->LOAD;
    BIT_BAND_PER(TIMEOUT_TMR->CTRL, TMR_CTRL_ON_Msk) = 1;
    timeout_start = TIMEOUT_TMR->VALUE;
    // while (((UART_PORT->DATA >> UART_PIN_RX_POS) & 1) != value ) {
    while (BIT_BAND_PER(UART_PORT->DATA,(1 << UART_PIN_RX_POS)) != value ) {
        if(timeout_start - TIMEOUT_TMR->VALUE > (SYSCLK/1000 * UART_TIMEOUT)){
            return -1;
        }
    };
    return 0;
}
int boot_init()
{
    uint32_t ticks_counted;
    uint32_t baud_i;
    uint32_t baud_f;
    
    DBG_PRINT(0x01);

    //Waiting for the start bit - 0

    if(wait_uart_rx(0) < 0){
        return -1;
    }
    //turn on timer
    UART_TMR->LOAD = 0xffffffffu;
    UART_TMR->VALUE = UART_TMR->LOAD;
    BIT_BAND_PER(UART_TMR->CTRL, TMR_CTRL_ON_Msk) = 1;
    //We are waiting for the start of a group of seven 1 (0x7F)
    if(wait_uart_rx(1) < 0){
        return -1;
    }
    
    ticks_counted = UART_TMR->VALUE;
    //Waiting for bit - 0
    if(wait_uart_rx(0) < 0){
        return -1;
    }

    ticks_counted -= UART_TMR->VALUE;
    UART_TMR->CTRL_bit.ON = 0;
    //calculate baudrate
    baud_i = ticks_counted / (16 * 7);
    baud_f = (uint32_t)((ticks_counted / (16.0f * 7.0f) - baud_i) * 64 + 0.5f);

    //Waiting for stop bit - 1
    if(wait_uart_rx(1) < 0){
        return -1;
    }
    //turn on UART
    UART->IBRD = baud_i;
    UART->FBRD = baud_f;
    UART->LCRH = (1 << UART_LCRH_FEN_Pos) | (3 << UART_LCRH_WLEN_Pos);
    UART->CR = (1 << UART_CR_RXE_Pos) | (1 << UART_CR_TXE_Pos) | (1 << UART_CR_UARTEN_Pos);
    //transmit the device signature with bytes swapped
    UART->DR = (PACKET_DEVICE_SIGN & 0xFF00) >> 8;
    UART->DR = PACKET_DEVICE_SIGN & 0x00FF;
    
    return 0;
}

void boot_exit()
{
    // GPIOB->DENSET = 1 << 5;
    // GPIOB->OUTENSET = 1 << 5;
    // GPIOB->DATAOUTCLR = 1 << 5;
    // while (1);
    
    flash_disable_boot();
    NVIC_SystemReset();
}

__attribute__((noreturn)) void boot_core()
{
    Packet_TypeDef packet;

    DBG_PRINT(0x02);
    packet_fifo_init();
    //send a message about readiness to accept commands
    packet.cmd_code = CMD_NONE;
    packet.tmp_data8[0] = MSG_READY;
    msg_cmd(&packet);

    while (1) {
        packet_receive(&packet);
        DBG_PRINT(0x03);
        DBG_PRINT(packet.cmd_code);
        switch (packet.cmd_code) {
        // Get commands
        case CMD_GET_INFO:
            get_info_cmd(&packet);
            break;
        case CMD_GET_CFGWORD:
            get_cfgword_cmd(&packet);
            break;
        // Set commands
        case CMD_SET_CFGWORD:
           set_cfgword_cmd(&packet);
           break;
        // Write commands
        case CMD_WRITE_PAGE:
            write_page_cmd(&packet);
            break;
        // Read commands
        case CMD_READ_PAGE:
            read_page_cmd(&packet);
            break;
        // Erase commands
        case CMD_ERASE_FULL:
            erase_cmd(&packet);
            break;
        case CMD_ERASE_PAGE:
            erase_cmd(&packet);
            break;
        // Exit
        case CMD_EXIT:
            exit_cmd(&packet);
            break;
        case CMD_NONE:
            msg_cmd(&packet);
            break;
        default:
            break;
        }
    }
}

void msg_cmd(Packet_TypeDef* packet)
{
    if (packet->cmd_code == CMD_NONE)
        packet->data_n = 4;
    packet->tmp_data8[1] = packet->cmd_code;
    packet->tmp_data8[2] = PACKET_EMPTY_DATA;
    packet->tmp_data8[3] = PACKET_EMPTY_DATA;
    packet->cmd_code = CMD_MSG;

    packet_transmit(packet);
}

void get_info_cmd(Packet_TypeDef* packet)
{
    uint16_t rx_crc;

    rx_crc = packet_fifo_read_u16();

    if (packet->crc != rx_crc) {
        packet->tmp_data8[0] = MSG_ERR_CRC;
        packet->data_n = 4;
    } else {
        packet->tmp_data8[0] = MSG_OK;
        packet->tmp_data32[1] = SIU->CHIPID;
        packet->tmp_data32[2] = SCB->CPUID;
        packet->tmp_data32[3] = BOOT_VER;
        size_t boot_name_len = sizeof(BOOT_NAME) + 1;
        memcpy(&(packet->tmp_data32[4]), BOOT_NAME, boot_name_len);
        packet->data_n = 16 + boot_name_len;
    }

    msg_cmd(packet);
}

void get_cfgword_cmd(Packet_TypeDef* packet)
{
    uint16_t rx_crc;
    uint32_t data[2];

    rx_crc = packet_fifo_read_u16();

    if (packet->crc != rx_crc) {
        packet->tmp_data8[0] = MSG_ERR_CRC;
        packet->data_n = 4;
    } else {
        packet->tmp_data8[0] = MSG_OK;
        flash_read(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR, data);
        packet->tmp_data32[1] = data[0];
        packet->data_n = 8;
    }

    msg_cmd(packet);
}

void set_cfgword_cmd(Packet_TypeDef* packet)
{
    uint32_t cfgword;
    uint16_t rx_crc;
    uint16_t calc_crc;
    uint32_t page_arr[FLASH_NVR_PAGE_SIZE_BYTES / 8][2];
    uint32_t data[2];
    uint32_t modify_en;

    flash_read(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR, data);
    modify_en = (data[0] & CFGWORD_NVRWE_MSK) >> CFGWORD_NVRWE_POS;

    cfgword = packet_fifo_read_u32();
    calc_crc = crc_upd_u32(packet->crc, cfgword);

    rx_crc = packet_fifo_read_u16();

    packet->data_n = 8;
    if (calc_crc != rx_crc)
        packet->tmp_data8[0] = MSG_ERR_CRC;
    else if (!modify_en)
        packet->tmp_data8[0] = MSG_FAIL;
    else {
        //read the whole page
        for (uint32_t i = 0; i < FLASH_NVR_PAGE_SIZE_BYTES / 8; i++) {
            flash_read(FLASH_NVR_CFGWORD_OFFSET + i * 8, FLASH_NVR, (uint32_t*)page_arr[i]);
        }
        //modify the CFGWORD, erase and write back
        page_arr[0][0] = cfgword;
        flash_erase_page(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR);
        for (uint32_t i = 0; i < FLASH_NVR_PAGE_SIZE_BYTES / 8; i++) {
            if ((page_arr[i][0] != 0xFFFFFFFF) || (page_arr[i][1] != 0xFFFFFFFF))
                flash_write(FLASH_NVR_CFGWORD_OFFSET + i * 8, FLASH_NVR, (uint32_t*)page_arr[i]);
        }
        // //verfi
        // packet->tmp_data8[0] = MSG_OK;
        // for (uint32_t i = 0; i < FLASH_NVR_PAGE_SIZE_BYTES / 8; i++) {
        //     flash_read(FLASH_NVR_CFGWORD_OFFSET + i * 8, FLASH_NVR, data);
        //     if ((page_arr[i][0] != data[0]) || (page_arr[i][1] != data[1]))
        //         packet->tmp_data8[0] = MSG_FAIL;
        // }
    }

    packet->tmp_data32[1] = cfgword;

    msg_cmd(packet);
}

void write_page_cmd(Packet_TypeDef* packet)
{
    uint32_t rx_data;
    uint8_t cfg;
    uint32_t addr;
    uint32_t addr_i;
    uint32_t flash_type;
    uint32_t erase_option;
    uint32_t data[2];
    uint16_t rx_crc;
    uint16_t calc_crc;
    uint32_t modify_en;

    //read the address, determine the required flash type and page number, then erase it if necessary
    rx_data = packet_fifo_read_u32();

    cfg = (uint8_t)(rx_data >> 24);

    //determine the type of flash and whether it can be written
    flash_type = (FlashType_TypeDef)((cfg & CMD_WRITE_PAGE_OPT_NVR_MSK) >> CMD_WRITE_PAGE_OPT_NVR_POS);
    flash_read(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR, data);
    if (flash_type == FLASH_MAIN)
        modify_en = (data[0] & CFGWORD_FLASHWE_MSK) >> CFGWORD_FLASHWE_POS;
    else
        modify_en = (data[0] & CFGWORD_NVRWE_MSK) >> CFGWORD_NVRWE_POS;

    //should erase before writing
    erase_option = (cfg & CMD_WRITE_PAGE_OPT_ERASE_MSK) >> CMD_WRITE_PAGE_OPT_ERASE_POS;

    addr = rx_data & ~(FLASH_PAGE_SIZE_BYTES - 1) & 0x00FFFFFF;

    //bootloader modification protection
    modify_en &= !((flash_type == FLASH_NVR) && (addr < (FLASH_PAGE_SIZE_BYTES * 3)));

    if (erase_option && modify_en)
        flash_erase_page(addr, flash_type);
    calc_crc = crc_upd_u32(packet->crc, rx_data);

    //read 8 bytes of data and write the whole page
    addr_i = addr;
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE_BYTES / 8; i++) {
        data[0] = packet_fifo_read_u32();
        data[1] = packet_fifo_read_u32();
        if (modify_en)
            flash_write(addr_i, flash_type, data);
        calc_crc = crc_upd_u32(calc_crc, data[0]);
        calc_crc = crc_upd_u32(calc_crc, data[1]);
        addr_i += 8;
    }

    rx_crc = packet_fifo_read_u16();

    packet->data_n = 8;
    if (calc_crc != rx_crc)
        packet->tmp_data8[0] = MSG_ERR_CRC;
    else if (!modify_en)
        packet->tmp_data8[0] = MSG_FAIL;
    else
        packet->tmp_data8[0] = MSG_OK;

    packet->tmp_data32[1] = rx_data;

    msg_cmd(packet);
}

void read_page_cmd(Packet_TypeDef* packet)
{
    uint32_t rx_data;
    uint8_t cfg;
    uint32_t addr;
    uint32_t addr_i;
    uint32_t flash_type;
    uint32_t data[2];
    uint16_t rx_crc;
    uint16_t calc_crc;
    uint32_t read_en;


    flash_read(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR, data);

    //read the address, determine the required flash type and page number, then erase it if necessary
    rx_data = packet_fifo_read_u32();

    cfg = (uint8_t)(rx_data >> 24);
    flash_type = (FlashType_TypeDef)((cfg & CMD_READ_PAGE_OPT_NVR_MSK) >> CMD_READ_PAGE_OPT_NVR_POS);
    //can HOST read flash
    if (flash_type == FLASH_MAIN){
        read_en = (data[0] & CFGWORD_FLASHRE_MSK) >> CFGWORD_FLASHRE_POS;
    }
    else{
        read_en = (data[0] & CFGWORD_NVRRE_MSK) >> CFGWORD_NVRRE_POS;
    }

    addr = rx_data & ~(FLASH_PAGE_SIZE_BYTES - 1) & 0x00FFFFFF;
    //bootloader read protection
    read_en &= !((flash_type == FLASH_NVR) && (addr < (FLASH_PAGE_SIZE_BYTES * 3)));

    calc_crc = crc_upd_u32(packet->crc, rx_data);
    rx_crc = packet_fifo_read_u16();

    packet->data_n = 8;
    if (calc_crc != rx_crc)
        packet->tmp_data8[0] = MSG_ERR_CRC;
    else if (!read_en)
        packet->tmp_data8[0] = MSG_FAIL;
    else {
        addr_i = addr;
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE_BYTES / 8; i++) {
            flash_read(addr_i, flash_type, data);
            packet->tmp_data32[2 + i * 2] = data[0];
            packet->tmp_data32[2 + i * 2 + 1] = data[1];
            addr_i += 8;
        }
        packet->tmp_data8[0] = MSG_OK;
        packet->data_n += FLASH_PAGE_SIZE_BYTES;
    }

    packet->tmp_data32[1] = rx_data;

    msg_cmd(packet);
}

void erase_cmd(Packet_TypeDef* packet)
{
    uint32_t rx_data;
    uint8_t cfg;
    uint32_t addr;
    uint32_t flash_type;
    uint16_t rx_crc;
    uint16_t calc_crc;
    uint32_t data[2];
    uint32_t modify_en;

    rx_data = packet_fifo_read_u32();
    cfg = (uint8_t)(rx_data >> 24);

    //determine the type of flash and whether the host can erase it
    flash_type = (FlashType_TypeDef)((cfg & CMD_WRITE_PAGE_OPT_NVR_MSK) >> CMD_WRITE_PAGE_OPT_NVR_POS);
    flash_read(FLASH_NVR_CFGWORD_OFFSET, FLASH_NVR, data);
    if (flash_type == FLASH_MAIN)
        modify_en = (data[0] & CFGWORD_FLASHWE_MSK) >> CFGWORD_FLASHWE_POS;
    else
        modify_en = (data[0] & CFGWORD_NVRWE_MSK) >> CFGWORD_NVRWE_POS;

    addr = rx_data & ~(FLASH_PAGE_SIZE_BYTES - 1) & 0x00FFFFFF;

    //bootloader erasure protection
    modify_en &= !((flash_type == FLASH_NVR) && (addr < (FLASH_PAGE_SIZE_BYTES * 3)));

    calc_crc = crc_upd_u32(packet->crc, rx_data);
    rx_crc = packet_fifo_read_u16();

    packet->data_n = 8;
    if (calc_crc != rx_crc)
        packet->tmp_data8[0] = MSG_ERR_CRC;
    else if (!modify_en)
        packet->tmp_data8[0] = MSG_FAIL;
    else {
        if(packet->cmd_code == CMD_ERASE_FULL){
            flash_erase_full();
        }else{
            flash_erase_page(addr, flash_type);
        }
        
        packet->tmp_data8[0] = MSG_OK;
    }

    packet->tmp_data32[1] = rx_data;

    msg_cmd(packet);
}

void exit_cmd(Packet_TypeDef* packet)
{
    uint16_t rx_crc;
    uint16_t calc_crc;

    calc_crc = packet->crc;
    rx_crc = packet_fifo_read_u16();

    packet->data_n = 4;
    if (calc_crc != rx_crc)
        packet->tmp_data8[0] = MSG_ERR_CRC;
    else
        packet->tmp_data8[0] = MSG_OK;

    msg_cmd(packet);
    while (packet_transmit_status_busy()) {
    };

    boot_exit();
}
