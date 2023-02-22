/**
 * \file            boot_packet.h
 * \brief           Functions for working with packets.
 *                  Must be initialized before use using packet_init().
 * \copyright       DC Vostok Vladivostok 2023
 */


#ifndef BOOT_PACKET_H
#define BOOT_PACKET_H

#include "boot_conf.h"

// clang-format off
//Command write page flash options
#define CMD_WRITE_PAGE_OPT_ERASE_POS    6
#define CMD_WRITE_PAGE_OPT_ERASE_MSK    (1<<CMD_WRITE_PAGE_OPT_ERASE_POS)
#define CMD_WRITE_PAGE_OPT_NVR_POS      7
#define CMD_WRITE_PAGE_OPT_NVR_MSK      (1<<CMD_WRITE_PAGE_OPT_NVR_POS)
#define CMD_READ_PAGE_OPT_NVR_POS       CMD_WRITE_PAGE_OPT_NVR_POS
#define CMD_READ_PAGE_OPT_NVR_MSK       CMD_WRITE_PAGE_OPT_NVR_MSK
// clang-format on

/**
 * \brief           Command codes
 */
typedef enum {
    CMD_GET_INFO = 0x35,    /*!< Get info about CHIPID, CPUID, BOOT_VER, and BOOT_NAME */
    CMD_GET_CFGWORD = 0x3A, /*!< Get config word CFGWORD */
    CMD_SET_CFGWORD = 0x65, /*!< Set config word CFGWORD */
    CMD_WRITE_PAGE = 0x9A, /*!< Write page of flash memory */
    CMD_READ_PAGE = 0xA5,  /*!< Read page of flash memory */
    CMD_ERASE_FULL = 0xC5, /*!< full erase of flash memory*/
    CMD_ERASE_PAGE = 0xCA, /*!< Page erase of flash memory*/
    CMD_NONE = 0x00, 
    CMD_EXIT = 0xF5,       /*!< Exit from bootloader*/
    CMD_MSG = 0xFA,        /*!< Message packet */
} CmdCode_TypeDef;

/**
 * \brief           Message codes for CMD_MSG
 * \note            Used as status code in packets
 */
typedef enum {
    MSG_NONE,
    MSG_ERR_CMD,
    MSG_ERR_CRC,
    MSG_READY,
    MSG_OK,
    MSG_FAIL
} MsgCode_TypeDef;

/**
 * \brief           Packet struct
 */
typedef struct
{
    CmdCode_TypeDef cmd_code;
    uint16_t data_n;
    uint16_t crc;
    union { /*!< reserved bytes */
        uint8_t tmp_data8[PACKET_TMP_DATA_BYTES];
        uint16_t tmp_data16[PACKET_TMP_DATA_BYTES / 2];
        uint32_t tmp_data32[PACKET_TMP_DATA_BYTES / 4];
    };
} Packet_TypeDef;

/**
 * \brief           Init fifo
 */
RAMFUNC void packet_fifo_init();

/**
 * \brief           Reading byte from packet fifo
 * 
 * \return          Byte of data 
 */
RAMFUNC uint8_t packet_fifo_read();

/**
 * \brief           Wrapper for reading 32-bit values from packet fifo
 * 
 * \return          32-bit data 
 */
RAMFUNC uint32_t packet_fifo_read_u32();

/**
 * \brief           Wrapper for reading 16-bit values from packet fifo
 * 
 * \return          16-bit data 
 */
RAMFUNC uint16_t packet_fifo_read_u16();

/**
 * \brief           Find and read packet from FIFO.  
 * 
 * \param[out]      rx_packet: Read packet
 */ 
RAMFUNC void packet_receive(Packet_TypeDef* cmd);

/**
 * \brief           Transmit packet
 * \param[in]       tx_packet: Transmit packet
 */ 
RAMFUNC void packet_transmit(Packet_TypeDef* tx_packet);

/**
 * \brief           Checking the status of sending the current package
 * \param[in]       data: Byte of data
 */ 
RAMFUNC uint32_t packet_transmit_status_busy();

/**
 * \brief           Update CRC16 value
 * 
 * \param[in]       crc_in: Current CRC16 value
 * \param[in]       data: Byte of data 
 * \return          Updated CRC16 value
 */ 
RAMFUNC uint16_t crc_upd(uint16_t crc_in, uint8_t data);

/**
 * \brief           Wrapper for calculating CRC16 for 32-bit data
 * 
 * \param[in]       crc_in: Current CRC16 value
 * \param[in]       data: 32-bit data 
 * \return          Updated CRC16 value
 */ 
RAMFUNC uint16_t crc_upd_u32(uint16_t crc_in, uint32_t data);

/**
 * \brief           Wrapper for calculating CRC16 for 16-bit data
 * 
 * \param[in]       crc_in: Current CRC16 value
 * \param[in]       data: 16-bit data 
 * \return          Updated CRC16 value
 */ 
RAMFUNC uint16_t crc_upd_u16(uint16_t crc_in, uint16_t data);

#endif //BOOT_PACKET_H
