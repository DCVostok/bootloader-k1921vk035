#include "boot_core.h"

static void DebugInit()
{
#ifdef DEBUG
    // configure service outputs for output
    RCU->HCLKCFG = DBG_PORT_EN;
    RCU->HRSTCFG = DBG_PORT_EN;
    DBG_PORT->DENSET = DBG_INFO_MSK << 8;
    DBG_PORT->OUTENSET = DBG_INFO_MSK << 8;
#endif
}

static void FPUInit()
{
    SCB->CPACR = 0x00F00000;
    __DSB();
    __ISB();
    for (uint8_t i = 0; i < 15; ++i){
      __NOP();
    }
}

static void ClockInit()
{
    //Set up PLL at 100 MHz (from internal 8 MHz)
    RCU->PLLCFG = (RCU_PLLCFG_REFSRC_OSICLK << RCU_PLLCFG_REFSRC_Pos) |
                  (1 << RCU_PLLCFG_N_Pos) |
                  (25 << RCU_PLLCFG_M_Pos) |
                  (1 << RCU_PLLCFG_OD_Pos) |
                  (1 << RCU_PLLCFG_OUTEN_Pos);
    //Waiting for PLL to stabilize
    while (!RCU->PLLCFG_bit.LOCK)
        ;
    // Set the number of waitstate for the flash drive = 4 (4*30 = 120 MHz max)
    MFLASH->CTRL = 3 << MFLASH_CTRL_LAT_Pos;
    // Change system frequency to PLL
    RCU->SYSCLKCFG = RCU_SYSCLKCFG_SYSSEL_PLLCLK << RCU_SYSCLKCFG_SYSSEL_Pos;
    //SystemCoreClockUpdate();

}

static void UartInit()
{
    UART_PORT->ALTFUNCSET = UART_PINS_MSK;
    UART_PORT->DENSET = UART_PINS_MSK;

    RCU->UARTCFG[UART_NUM].UARTCFG = (RCU_UARTCFG_UARTCFG_CLKSEL_PLLCLK << RCU_UARTCFG_UARTCFG_CLKSEL_Pos) |
                                     (1 << RCU_UARTCFG_UARTCFG_CLKEN_Pos) |
                                     (1 << RCU_UARTCFG_UARTCFG_RSTDIS_Pos);
     UART->IFLS = UART_IFLS_RXIFLSEL_Lvl18 << UART_IFLS_RXIFLSEL_Pos |
                  UART_IFLS_RXIFLSEL_Lvl18 << UART_IFLS_TXIFLSEL_Pos;
    UART->IMSC = UART_MIS_RXMIS_Msk;
    NVIC_EnableIRQ(UART_RX_IRQn);
}


void GpioInit(){
    BIT_BAND_PER(RCU->HCLKCFG, RCU_HCLKCFG_GPIOAEN_Msk) = 1;
    BIT_BAND_PER(RCU->HRSTCFG, RCU_HCLKCFG_GPIOAEN_Msk) = 1;
    BIT_BAND_PER(RCU->HCLKCFG, RCU_HCLKCFG_GPIOBEN_Msk) = 1;
    BIT_BAND_PER(RCU->HRSTCFG, RCU_HCLKCFG_GPIOBEN_Msk) = 1;
    BOOTEN_PORT->DENSET = BOOTEN_PIN_MSK;
    BOOTEN_PORT->PULLMODE = 0b01 << (BOOTEN_PIN_POS * 2); // enable Pull Up
}
void TimersInit(){
    BIT_BAND_PER(RCU->PRSTCFG, UART_TMR_EN_Msk) = 1;
    BIT_BAND_PER(RCU->PCLKCFG_bit, UART_TMR_EN_Msk) = 1;
    BIT_BAND_PER(RCU->PRSTCFG, TIMEOUT_TMR_EN_Msk) = 1;
    BIT_BAND_PER(RCU->PCLKCFG_bit, TIMEOUT_TMR_EN_Msk) = 1;
}

void PeriphInit()
{
    GpioInit();
    DebugInit();
    FPUInit();
    ClockInit();
    GpioInit();
    UartInit();
    TimersInit();
}


int main()
{
    PeriphInit();
    
    if ((BOOTEN_PORT->DATA & BOOTEN_PIN_MSK) > 0){
        boot_exit();
    }

    
    if(boot_init() < 0){
        boot_exit();
    }

    boot_core();
    return 0;
}
