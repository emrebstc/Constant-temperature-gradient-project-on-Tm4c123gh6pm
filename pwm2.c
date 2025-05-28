#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "delayms.h"

// BTS7960 PWM ve yön pinleri
#define RPWM_PIN     GPIO_PIN_4   // PE4 > PWM çýkýþý
#define LPWM_PIN     GPIO_PIN_5   // PE5 > LOW
#define R_EN_PIN     GPIO_PIN_6   // PC6 > HIGH
#define L_EN_PIN     GPIO_PIN_7   // PC7 > HIGH


void PWM_Init2(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    // Yön pinleri
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN);
    GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);

    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, LPWM_PIN);
    GPIOPinWrite(GPIO_PORTE_BASE, LPWM_PIN, 0);

    GPIOPinConfigure(GPIO_PE4_M0PWM4);
    GPIOPinTypePWM(GPIO_PORTE_BASE, RPWM_PIN);

    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, SysCtlClockGet() / 15000); // 15 kHz

    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
}

void SetPWM_Duty2(float duty) {
    uint32_t period = PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2);
    uint32_t pulse = (uint32_t)((period * duty) / 100.0f);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, pulse);
}
