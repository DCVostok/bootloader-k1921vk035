/**
 * \file            boot_flash.h
 * \brief           Functions for working with flash memory.
 * \copyright       DC Vostok Vladivostok 2023
 */


#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include "boot_conf.h"

#define FLASH_MAGICKEY_CONST        0xC0DEUL
#define FLASH_PAGE_SIZE_BYTES       1024UL
#define FLASH_PAGE_SIZE_BYTES_LOG2  10UL
#define FLASH_PAGE_TOTAL            64UL
#define FLASH_TOTAL_BYTES           (FLASH_PAGE_SIZE_BYTES*FLASH_PAGE_TOTAL)
#define FLASH_NVR_PAGE_SIZE_BYTES   FLASH_PAGE_SIZE_BYTES
#define FLASH_NVR_PAGE_TOTAL        4UL
#define FLASH_NVR_TOTAL_BYTES       (FLASH_NVR_PAGE_SIZE_BYTES*FLASH_NVR_PAGE_TOTAL)
#define FLASH_NVR_CFGWORD_OFFSET    (3*FLASH_NVR_PAGE_SIZE_BYTES)

#define CFGWORD_FLASHWE_POS         3
#define CFGWORD_NVRWE_POS           2
#define CFGWORD_NVRRE_POS           6
#define CFGWORD_FLASHRE_POS         7
#define CFGWORD_FLASHWE_MSK         (1<<CFGWORD_FLASHWE_POS)
#define CFGWORD_NVRWE_MSK           (1<<CFGWORD_NVRWE_POS)
#define CFGWORD_NVRRE_MSK           (1<<CFGWORD_NVRRE_POS)
#define CFGWORD_FLASHRE_MSK         (1<<CFGWORD_FLASHRE_POS)

typedef enum {
    FLASH_MAIN = 0,
    FLASH_NVR = 1
} FlashType_TypeDef;

typedef enum {
    FLASH_RD = MFLASH_CMD_RD_Msk,
    FLASH_WR = MFLASH_CMD_WR_Msk,
    FLASH_ERSEC = MFLASH_CMD_ERSEC_Msk,
    FLASH_ERALL = MFLASH_CMD_ERALL_Msk
} FlashCmd_TypeDef;

/**
 * \brief           Read 2x32-bit data (8 byte) from flash memory
 * \param[in]       addr: flash memory address 
 * \param[in]       ftype: Type of flash memory
 * \param[out]      data: array with size 2
 */ 
RAMFUNC void flash_read(uint32_t addr, FlashType_TypeDef ftype, uint32_t* data);

/**
 * \brief           Write 2x32-bit data (8 byte) to flash memory
 * \param[in]       addr: flash memory address 
 * \param[in]       ftype: Type of flash memory
 * \param[in]      data: array with size 2
 */ 
RAMFUNC void flash_write(uint32_t addr, FlashType_TypeDef ftype, const uint32_t* data);

/**
 * \brief           Erase page of flash memory
 * \param[in]       addr: flash memory address 
 * \param[in]       ftype: Type of flash memory
 */
RAMFUNC void flash_erase_page(uint32_t addr, FlashType_TypeDef ftype);

/**
 * \brief           Full erase of flash memory
 */
RAMFUNC void flash_erase_full();

/**
 * \brief           Disabling startup from boot memory after the next software reset
 */
RAMFUNC void flash_disable_boot();

#endif //BOOT_FLASH_H
