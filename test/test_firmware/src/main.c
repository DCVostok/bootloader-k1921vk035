#include "plib035.h"
#include "retarget_conf.h"

#define LED_PIN (GPIO_Pin_5)
#define LED_PORT (GPIOB)

void periph_init()
{
    SystemCoreClockUpdate();
    retarget_init(); //UART0 used for printf
    SysTick_Config(10000000);//5 Hz
    
    RCU_AHBClkCmd(RCU_AHBClk_GPIOA, ENABLE);
    RCU_AHBRstCmd(RCU_AHBRst_GPIOA, ENABLE);
    RCU_AHBClkCmd(RCU_AHBClk_GPIOB, ENABLE);
    RCU_AHBRstCmd(RCU_AHBRst_GPIOB, ENABLE);

    GPIO_Init_TypeDef gpio_init_struct;
    GPIO_StructInit(&gpio_init_struct);
    gpio_init_struct.Pin = LED_PIN;
    gpio_init_struct.Out = ENABLE;
    gpio_init_struct.AltFunc = DISABLE;
    gpio_init_struct.OutMode = GPIO_OutMode_PP;
    gpio_init_struct.DriveMode = GPIO_DriveMode_HighFast;
    gpio_init_struct.Digital = ENABLE;
    GPIO_Init(LED_PORT,&gpio_init_struct);
}

volatile int a = 0;
//-- Main ----------------------------------------------------------------------
int main()
{
    periph_init();
    printf("All periph inited\n");
    printf("CPU frequency is %lu MHz\n", SystemCoreClock / (int)1E6);

    while(1){
       // __WFI();
    }
    return 0;
}

void SysTick_Handler()
{
    GPIO_ToggleBits(LED_PORT, LED_PIN);
    printf("led state %d\n", GPIO_ReadBit(LED_PORT, LED_PIN));
    a++;// You can use debug breakpoint there to see value of variable
}