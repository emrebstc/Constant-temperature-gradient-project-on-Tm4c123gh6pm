#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "inc/tm4c123gh6pm.h"
#include "DelayMs.h"

#define MAX6675_CS1 GPIO_PIN_2
#define MAX6675_PORT1 GPIO_PORTE_BASE
#define MAX6675_SSI_BASE1 SSI0_BASE


void MAX6675_Init1(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);  // PE2 için

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0));

    // SSI0 için PA2 = CLK, PA4 = MISO
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_4);

    // CS pini olarak PE2 tanýmlanýr
    GPIOPinTypeGPIOOutput(MAX6675_PORT1, MAX6675_CS1);
    GPIOPinWrite(MAX6675_PORT1, MAX6675_CS1, MAX6675_CS1);  // CS HIGH (bekleme)

    // SSI yapýlandýrmasý
    SSIConfigSetExpClk(MAX6675_SSI_BASE1, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER,
                       1000000, 16);
    SSIEnable(MAX6675_SSI_BASE1);
}


float MAX6675_ReadTemp1(void) {
    uint32_t data;
    GPIOPinWrite(MAX6675_PORT1, MAX6675_CS1, 0);
    DelayMs(1);

    SSIDataPut(MAX6675_SSI_BASE1, 0x0000);
    while(SSIBusy(MAX6675_SSI_BASE1));
    SSIDataGet(MAX6675_SSI_BASE1, &data);

    GPIOPinWrite(MAX6675_PORT1, MAX6675_CS1, MAX6675_CS1);

    if(data & 0x04) return -1.0f;
    return ((data >> 3) & 0x0FFF) * 0.25f;
}
