/**
 * \file            boot_conf.h
 * \brief           General bootloader configuration file: global constants, general macros, types
 * \copyright       DC Vostok Vladivostok 2023
 */

#ifndef BOOT_CONF_H
#define BOOT_CONF_H

#include <stdio.h>
#include "K1921VK035.h"
#include "system_K1921VK035.h"


//Global configuration
#define RAMFUNC         __attribute__( (long_call, section(".ramfunc") ) )
#define BOOT_VER_MAJOR  0x0001
#define BOOT_VER_MINOR  0x0003
#define BOOT_VER        ((BOOT_VER_MAJOR<<16)|BOOT_VER_MINOR)
#define BOOT_NAME       "K1921VK035_BOOTLOADER"
#define SYSCLK          100000000

/**
 * \note           BOOTEN pin used for activate bootloader 
 */
#define BOOTEN_PORT GPIOA /*!< Port of  BOOTEN pin */
#define BOOTEN_PIN_POS (7) /*!< Pin num of BOOTEN pin */
#define BOOTEN_PIN_MSK (1 << BOOTEN_PIN_POS) /*!< Pin num of BOOTEN pin */
//Debug
#define DBG_PORT        GPIOA
#define DBG_PORT_EN     RCU_HCLKCFG_GPIOAEN_Msk
#define DBG_INFO_MSK    0xFF
#define DBG_INFO_POS    8
#define DBG_INFO        DBG_PORT->DATAOUT
#if defined (DEBUG)
    #define DBG_PRINT(MSG)   DBG_INFO = MSG<<DBG_INFO_POS
#else
    #define DBG_PRINT(MSG)    ((void)0);
#endif

/**
 * \brief           UART for communicate with HOST
 */
#define UART                UART0
#define UART_NUM            0
#define UART_PORT           GPIOB
#define UART_PORT_EN        RCU_HCLKCFG_GPIOBEN_Msk
#define UART_PIN_RX_POS     11
#define UART_PIN_TX_POS     10
#define UART_PINS_MSK       ((1<<UART_PIN_RX_POS) | (1<<UART_PIN_TX_POS))
#define UART_RX_IRQHandler  UART0_RX_IRQHandler
#define UART_RX_IRQn        UART0_RX_IRQn
#define UART_TIMEOUT        (500)//ms

/**
 * \brief           Timer for calculate uart speed
 */
#define UART_TMR TMR1
#define UART_TMR_EN_Msk RCU_PRSTCFG_TMR1EN_Msk

/**
 * \brief           Timer for timeout 
 */
#define TIMEOUT_TMR TMR0
#define TIMEOUT_TMR_EN_Msk RCU_PRSTCFG_TMR0EN_Msk

/**
 * \brief           Packet parser values
 */
#define PACKET_FIFO_BYTES       8192
#define PACKET_HOST_SIGN        0x5C81
#define PACKET_DEVICE_SIGN      0x7EA3
#define PACKET_EMPTY_DATA       0x55
#define PACKET_TMP_DATA_BYTES   (1024+8)

#endif //BOOT_CONF_H
