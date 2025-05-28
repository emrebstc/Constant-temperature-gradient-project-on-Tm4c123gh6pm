#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "inc/tm4c123gh6pm.h"
#include "DelayMs.h"
#include "max6675-sogutma.h"

#define MAX6675_CS2       GPIO_PIN_1  // PD1
#define MAX6675_PORT2     GPIO_PORTD_BASE
#define MAX6675_SSI_BASE2  SSI1_BASE

void MAX6675_Init2(void) {
    // 1. SSI1 ve GPIOD clock'larýný etkinleþtir
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    // 2. PD0 ve PD2 pinlerini SSI1 için yapýlandýr
    GPIOPinConfigure(GPIO_PD0_SSI1CLK);  // CLK = PD0
    GPIOPinConfigure(GPIO_PD2_SSI1RX);   // MISO = PD2
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_2);

    // 3. CS pinini çýkýþ olarak ayarla (PD1)
    GPIOPinTypeGPIOOutput(MAX6675_PORT2, MAX6675_CS2);
    GPIOPinWrite(MAX6675_PORT2, MAX6675_CS2, MAX6675_CS2);  // CS HIGH (pasif)

    // 4. SSI1'i Mode 0, 1MHz, 16-bit veri olarak ayarla
    SSIConfigSetExpClk(MAX6675_SSI_BASE2, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 16);
    SSIEnable(MAX6675_SSI_BASE2);
}

float MAX6675_ReadTemp2(void) {
    uint32_t data = 0;

    // CS LOW (sensör aktif)
    GPIOPinWrite(MAX6675_PORT2, MAX6675_CS2, 0);
    DelayMs(1);  // 10µs bekle (MAX6675'nin hazýr olmasý için)

    // 16-bit dummy data gönder ve veriyi oku
    SSIDataPut(MAX6675_SSI_BASE2, 0x0000);
    while(SSIBusy(MAX6675_SSI_BASE2));  // Ýþlem bitene kadar bekle
    SSIDataGet(MAX6675_SSI_BASE2, &data);

    // CS HIGH (sensör pasif)
    GPIOPinWrite(MAX6675_PORT2, MAX6675_CS2, MAX6675_CS2);

    // Hata kontrolü (termokupl baðlý deðilse -1.0 döndür)
    if (data & 0x04) return -1.0f;

    // 12-bit veriyi oku ve 0.25 ile çarparak °C'ye çevir
    return ((data >> 3) & 0x0FFF) * 0.25f;
}
