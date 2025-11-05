#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "PIDheater.h"
#include "PIDcooler.h"
#include "DelayMs.h"
#include "pwm1.h"
#include "pwm2.h"
#include "max6675-isitma.h"
#include "max6675-sogutma.h"
#include "LCD_I2C.h"
#include "floattostring.h"

// PID parametreleri
#define PID_KP1 200.0f
#define PID_KI1 150.0f       //ısıtma
#define PID_KD1 0.5f
#define SETPOINT1 35.0f

#define PID_KP2 7.0f
#define PID_KI2 0.3f
#define PID_KD2 25.0f
#define SETPOINT2 8.0f
#define HYSTERESIS 0.5f

// L298N PWM ve yön pinleri
#define HEATER_PWM_PIN GPIO_PIN_3 // PB5 (M0PWM3)
#define HEATER_IN1 GPIO_PIN_2     // PB2
#define HEATER_IN2 GPIO_PIN_3     // PB3
#define HEATER2_PWM_PIN GPIO_PIN_2 // PB6 (M0PWM0)
#define HEATER2_IN3 GPIO_PIN_0     // PB0
#define HEATER2_IN4 GPIO_PIN_1     // PB1
#define GPIO_PORTB GPIO_PORTB_BASE

// BTS7960 PWM ve yön pinleri
#define RPWM_PIN     GPIO_PIN_4   // PE4 > PWM çıkışı
#define LPWM_PIN     GPIO_PIN_5   // PE5 > LOW
#define R_EN_PIN     GPIO_PIN_6   // PC6 > HIGH
#define L_EN_PIN     GPIO_PIN_7   // PC7 > HIGH

#define MODE_SWITCH_PORT GPIO_PORTA_BASE
#define MODE_SWITCH_PIN  GPIO_PIN_5


void Heater_GPIO_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    GPIOPinTypeGPIOOutput(GPIO_PORTB,
    HEATER_IN1 | HEATER_IN2 | HEATER2_IN3 | HEATER2_IN4);

    // Tek yönlü çalışma: IN1=1, IN2=0 | IN3=1, IN4=0
    GPIOPinWrite(GPIO_PORTB, HEATER_IN1, HEATER_IN1);
    GPIOPinWrite(GPIO_PORTB, HEATER_IN2, 0);
    GPIOPinWrite(GPIO_PORTB, HEATER2_IN3, HEATER2_IN3);
    GPIOPinWrite(GPIO_PORTB, HEATER2_IN4, 0);
}

void Turn_Off_Heating(void) {
    SetPWM_Duty1(0);
    GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2 | HEATER2_IN3 | HEATER2_IN4, 0);
}

void Turn_Off_Cooling(void) {
    SetPWM_Duty2(0);
    GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, 0);
}

void Mode_Switch_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeGPIOInput(MODE_SWITCH_PORT, MODE_SWITCH_PIN);
    GPIOPadConfigSet(MODE_SWITCH_PORT, MODE_SWITCH_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
bool Is_Heating_Mode(void) {
       // Switch OFF (GND'ye bağlı) ise ısıtma modu
       return (GPIOPinRead(MODE_SWITCH_PORT, MODE_SWITCH_PIN) == 0);
   }
int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2);

    MAX6675_Init1();
    MAX6675_Init2();
    PWM_Init1();
    PWM_Init2();
    Heater_GPIO_Init();
    I2C1_Init();
    LCD_Init();
    Mode_Switch_Init();

      PIDController1 pid1;
      PID_Init1(&pid1, PID_KP1, PID_KI1, PID_KD1, SETPOINT1);
      PIDController2 pid2;
      PID_Init2(&pid2, PID_KP2, PID_KI2, PID_KD2, SETPOINT2);

    float temperature1;
    float temperature2;
    char buffer[16];

    while(1) {

       uint32_t currentTime = SysCtlClockGet() / 16000;
        LCD_Clear();
        if (Is_Heating_Mode()) {

            Turn_Off_Cooling();
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
              temperature1 = MAX6675_ReadTemp1();
              DelayMs(10);
              ftoa(temperature1, buffer, 2);

              LCD_WriteAt(1, 0,"Isitma Aktif. ");
              LCD_WriteAt(0, 0,"Derece:");
              LCD_WriteAt(0, 8,buffer);
              LCD_WriteAt(0, 14,"C");

               if(temperature1 >= SETPOINT1) {
                                     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 2);
                                     SetPWM_Duty1(0); // Sıcaklık setpoint’i geçti, kapat

                                     pid1.outputSum = 0;

                                     GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2, 0);

                                     GPIOPinWrite(GPIO_PORTB, HEATER2_IN3 | HEATER2_IN4, 0);

                                 }

                   else {

                       GPIOPinWrite(GPIO_PORTB, HEATER_IN1, HEATER_IN1);

                       GPIOPinWrite(GPIO_PORTB, HEATER_IN2, 0);

                       GPIOPinWrite(GPIO_PORTB, HEATER2_IN3, HEATER2_IN3);

                       GPIOPinWrite(GPIO_PORTB, HEATER2_IN4, 0);

                       GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);

                       float output1 = PID_Compute1(&pid1, temperature1, currentTime);

                       SetPWM_Duty1(output1);

                   }

               }
        else {

        Turn_Off_Heating();
          GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);

        temperature2 = MAX6675_ReadTemp2();
        DelayMs(10);
        ftoa(temperature2, buffer, 2); 
        LCD_WriteAt(1, 0,"Sogutma Aktif. ");
        LCD_WriteAt(0, 0,"Derece:");
        LCD_WriteAt(0, 8,buffer);
        LCD_WriteAt(0, 14,"C");

            // Soğutma için histerezisli kontrol
            if (temperature2 > (SETPOINT2+HYSTERESIS)) {
                // Soğutmaya başla
                GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
                float output2 = PID_Compute2(&pid2, temperature2, currentTime);
                SetPWM_Duty2(output2);  
            }
            else if (temperature2 <= (SETPOINT2-HYSTERESIS)) {
                // Soğutmayı durdur
                SetPWM_Duty2(0);
                GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, 0);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
                pid2.outputSum = 0;
            }
        }
        DelayMs(400);
}
}           
