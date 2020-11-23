

/* own header files */
#include "XDKAppInfo.h"
#undef BCDS_MODULE_ID
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_GYROSCOPE
#include "External.h"
#include "XdkSensorHandle.h"

/* additional interface header files */
#include "BCDS_Basics.h"
#include "BCDS_CmdProcessor.h"
#include "FreeRTOS.h"
#include "timers.h"

#include "em_adc.h"
#include "em_gpio.h"
#include "BSP_BoardShared.h"

/* system header files */
#include <stdio.h>

/* local prototypes ********************************************************* */

/* constant definitions ***************************************************** */

/* local variables ********************************************************** */

/* global variables ********************************************************* */

char* processExternalData(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    ADC_InitSingle_TypeDef channelInit = ADC_INITSINGLE_DEFAULT;
    channelInit.reference = adcRef2V5;
    channelInit.resolution = adcRes12Bit;
    channelInit.input = adcSingleInpCh5;
    ADC_InitSingle(ADC0, &channelInit);

    uint32_t AdcSample = 0;

	while ((ADC0->STATUS & (ADC_STATUS_SINGLEACT)) && (BSP_UNLOCKED == ADCLock));
	        __disable_irq();

	        ADCLock = BSP_LOCKED;
	        __enable_irq();
	        ADC_Start(ADC0, adcStartSingle);

	        // Wait while conversion is in process
	        while (ADC0->STATUS & (ADC_STATUS_SINGLEACT));
	        AdcSample = 0xFFF & ADC_DataSingleGet(ADC0);

	        printf("Adc Single Sample %u \n\r",(unsigned int) AdcSample);

	        __disable_irq();
	        ADCLock = BSP_UNLOCKED;
	        __enable_irq();



    char  *buffer = calloc(255, sizeof(char));

	sprintf(buffer,"{\"sensor\":\"Moisture\",\"data\":[{\"mi\":\"%u\"}]}",
	    			(unsigned int) AdcSample);

    return (char*)buffer;
}


void externalSensorInit(void)
{

	GPIO_PinModeSet(gpioPortD, 5, gpioModeInputPull, 0);
	GPIO_PinOutClear(gpioPortD, 5);
}

