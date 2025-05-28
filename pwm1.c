#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"

void PWM_Init1(void) {
    // Gerekli peripheral'larý aktif et
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM1));

    // PF2 (PWM1_OUT_6) ve PF3 (PWM1_OUT_7) pinlerini PWM olarak ayarla
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
    GPIOPinConfigure(GPIO_PF3_M1PWM7);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // PF2: PWM Generator 3, PWM1_OUT_6
    // PF3: PWM Generator 3, PWM1_OUT_7 (ayný generator!)
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    uint32_t pwmPeriod = SysCtlClockGet() / 15000; // 15kHz
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, pwmPeriod);

    // PWM generator'ý baþlat
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);

    // PWM çýkýþlarýný aktif et (OUT_6 ve OUT_7)
    PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT | PWM_OUT_7_BIT, true);
}

void SetPWM_Duty1(float duty) {
    uint32_t period = PWMGenPeriodGet(PWM1_BASE, PWM_GEN_3);
    uint32_t pulseWidth = (uint32_t)((period * duty) / 100.0f);

    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, pulseWidth); // PF2
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, pulseWidth); // PF3
}
