/**
 * \file            boot_core.h
 * \brief           Bootloader core
 * \copyright       DC Vostok Vladivostok 2023
 */

#ifndef BOOT_CORE_H
#define BOOT_CORE_H

#include "boot_conf.h"
#include "bitbanding.h"


// return 0 if start UART byte received
// return -1 If a UART timeout has occurred

/**
 * \brief           Try receive start byte from HOST and calculate UART speed
 * \return          0 if start UART byte received  
 *                  -1 If a UART timeout has occurred
 */
int boot_init(); 

/**
 * \brief           Exit from bootloader to main firmware
 */
RAMFUNC void boot_exit();

/**
 * \brief           Starts bootloader core loop and receives packets from HOST
 */
RAMFUNC void boot_core();

#endif //BOOT_CORE_H
