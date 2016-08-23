#include "delay.h"
#include "system_stm32f10x.h"
#include "stm32f10x_tim.h"

void DelayUs(unsigned int dwTime)
{
    unsigned int dwCurCounter=0;                                
    unsigned int dwPreTickVal=SysTick->VAL;                     
    unsigned int dwCurTickVal;                                  
    dwTime=dwTime*(SystemCoreClock/1000000);    
    for(;;){
        dwCurTickVal=SysTick->VAL;
        if(dwCurTickVal<dwPreTickVal){
            dwCurCounter=dwCurCounter+dwPreTickVal-dwCurTickVal;
        }
        else{
            dwCurCounter=dwCurCounter+dwPreTickVal+SysTick->LOAD-dwCurTickVal;
        }
        dwPreTickVal=dwCurTickVal;
        if(dwCurCounter>=dwTime){
            return;
        }
    }
}
void DelayMs(unsigned int dwTime)
{
    unsigned int dwCurCounter=0;                                
    unsigned int dwPreTickVal=SysTick->VAL;                     
    unsigned int dwCurTickVal;                                  
    dwTime=dwTime*(SystemCoreClock/1000);    
    for(;;){
        dwCurTickVal=SysTick->VAL;
        if(dwCurTickVal<dwPreTickVal){
            dwCurCounter=dwCurCounter+dwPreTickVal-dwCurTickVal;
        }
        else{
            dwCurCounter=dwCurCounter+dwPreTickVal+SysTick->LOAD-dwCurTickVal;
        }
        dwPreTickVal=dwCurTickVal;
        if(dwCurCounter>=dwTime){
            return;
        }
    }
}

void init_delay(void) {
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 62500);
	NVIC_SetPriority(SysTick_IRQn, 0);
}

