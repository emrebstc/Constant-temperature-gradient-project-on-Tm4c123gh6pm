#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "DelayMs.h"
#include "pwm1.h"
#include "pwm2.h"
#include "max6675-isitma.h"
#include "max6675-sogutma.h"
#include "LCD_I2C.h"
#include "floattostring.h"


// Isýtýcý ve soðutucu pin tanýmlarý
#define HEATER_IN1 GPIO_PIN_2
#define HEATER_IN2 GPIO_PIN_3
#define HEATER2_IN3 GPIO_PIN_0
#define HEATER2_IN4 GPIO_PIN_1
#define GPIO_PORTB GPIO_PORTB_BASE

#define RPWM_PIN     GPIO_PIN_4
#define LPWM_PIN     GPIO_PIN_5
#define R_EN_PIN     GPIO_PIN_6
#define L_EN_PIN     GPIO_PIN_7

#define MODE_SWITCH_PORT GPIO_PORTA_BASE
#define MODE_SWITCH_PIN  GPIO_PIN_5

// BULANIK MANTIK KONTROL FONKSÝYONLARI

float Fuzzy_Heater_Output(float error) {
    if (error > 2.5f)
        return 100.0f;
    else if (error > 2.0f)
           return 90.0f;
    else if (error > 1.0f)
        return 80.0f;
    else
        return 70.0f;

}

float Fuzzy_Cooler_Output(float error) {
    if (error > 0.5f)
        return 100.0f;
    else if (error > -1.5f)
        return 75.0f;
    else
        return 0.0f;
}


void Heater_GPIO_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    GPIOPinTypeGPIOOutput(GPIO_PORTB, HEATER_IN1 | HEATER_IN2 | HEATER2_IN3 | HEATER2_IN4);
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
    return (GPIOPinRead(MODE_SWITCH_PORT, MODE_SWITCH_PIN) == 0);
}

  int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    MAX6675_Init1();
    MAX6675_Init2();
    PWM_Init1();
    PWM_Init2();
    Heater_GPIO_Init();
    I2C1_Init();
    LCD_Init();
    Mode_Switch_Init();

    float temperature1;
    float temperature2;
    char bufferheat[16];
    char buffercold[16];
    char buffer1[16];
    char buffer2[16];

    float SETPOINT1;
    float SETPOINT2;

    float error1;
    float error2;

    int i = 0;

    while(1) {
        LCD_Clear();

        if (Is_Heating_Mode()) {
            if (i==0){
                Turn_Off_Cooling();
            }
            i++;

            temperature1 = MAX6675_ReadTemp1();
            DelayMs(10);
            temperature2 = MAX6675_ReadTemp2();
            DelayMs(10);

            if (temperature1>=41.5f){

            }

            else {
               temperature2 = temperature2+2.0f;
            }



            SETPOINT1 = 44.0f;  // Hedef deðeri  isitma
            SETPOINT2 = 22.0f;   // Hedef deðeri soðutma

            ftoa(temperature1, bufferheat, 2);
            ftoa(temperature2, buffercold, 2);
            ftoa(SETPOINT1, buffer1, 2);
            ftoa(SETPOINT2, buffer2, 2);

                LCD_WriteAt(1, 0,buffer1);
                LCD_WriteAt(1, 5,"C");
                LCD_WriteAt(1, 6,"  -  ");
                LCD_WriteAt(1, 11,buffer2);
                LCD_WriteAt(1, 15,"C");
                LCD_WriteAt(0, 0,bufferheat);
                LCD_WriteAt(0, 5,"C");
                LCD_WriteAt(0, 6," --");
                LCD_WriteAt(0, 10,buffercold);
                LCD_WriteAt(0, 15,"C");


            error1 = SETPOINT1 - temperature1;

            if(temperature2<SETPOINT2){
                //Sogutma kapa
                SetPWM_Duty2(0);
                GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, 0);

                if(temperature1 >= SETPOINT1) {
                           //Isýtma kapa
                            SetPWM_Duty1(0);
                            GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2, 0);
                            GPIOPinWrite(GPIO_PORTB, HEATER2_IN3 | HEATER2_IN4, 0);
                        }
                else {
                            //Isýtma Ac
                           GPIOPinWrite(GPIO_PORTB, HEATER_IN1, HEATER_IN1);
                            GPIOPinWrite(GPIO_PORTB, HEATER_IN2, 0);
                            GPIOPinWrite(GPIO_PORTB, HEATER2_IN3, HEATER2_IN3);
                            GPIOPinWrite(GPIO_PORTB, HEATER2_IN4, 0);
                            float output1 = Fuzzy_Heater_Output(error1);
                            SetPWM_Duty1(output1);
                        }


            }
            else{
                if(temperature1 >= SETPOINT1) {
                                         //Isýtma kapa
                                         SetPWM_Duty1(0);
                                         GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2, 0);
                                         GPIOPinWrite(GPIO_PORTB, HEATER2_IN3 | HEATER2_IN4, 0);

                                         //Sogutucuyu ac

                                          GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);
                                          SetPWM_Duty2(90.0f);
                                                                       }

                else {
                             //Sogutucuyu ac
                                   GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);
                                   SetPWM_Duty2(90.0f);
                                     //Isýtma ac
                                        GPIOPinWrite(GPIO_PORTB, HEATER_IN1, HEATER_IN1);
                                         GPIOPinWrite(GPIO_PORTB, HEATER_IN2, 0);
                                         GPIOPinWrite(GPIO_PORTB, HEATER2_IN3, HEATER2_IN3);
                                         GPIOPinWrite(GPIO_PORTB, HEATER2_IN4, 0);
                                         float output1 = Fuzzy_Heater_Output(error1);
                                         SetPWM_Duty1(output1);
                                     }

            }



        }

        else {

            if (i==0){
                           Turn_Off_Heating();
                       }
                       i++;

                       temperature1 = MAX6675_ReadTemp1();
                       DelayMs(10);
                       temperature2 = MAX6675_ReadTemp2();
                       DelayMs(10);

                       if (temperature2>4.0f & temperature1<23.0f){

                       }

                       else {
                          temperature2 = temperature2-3.75f;
                       }

                       SETPOINT1 = 22.0f;  // Hedef deðeri  isitma
                       SETPOINT2 = 0.0f;   // Hedef deðeri soðutma

                       ftoa(temperature1, bufferheat, 2);
                       ftoa(temperature2, buffercold, 2);
                       ftoa(SETPOINT1, buffer1, 2);
                       ftoa(SETPOINT2, buffer2, 2);

                           LCD_WriteAt(1, 0,buffer1);
                           LCD_WriteAt(1, 5,"C");
                           LCD_WriteAt(1, 6,"  -  ");
                           LCD_WriteAt(1, 11,buffer2);
                           LCD_WriteAt(1, 15,"C");
                           LCD_WriteAt(0, 0,bufferheat);
                           LCD_WriteAt(0, 5,"C");
                           LCD_WriteAt(0, 6," --");
                           LCD_WriteAt(0, 10,buffercold);
                           LCD_WriteAt(0, 15,"C");


                         error2 = temperature2 - SETPOINT2;

                       if(temperature1<SETPOINT1){
                           //Isýtma ac
                           GPIOPinWrite(GPIO_PORTB, HEATER_IN1, HEATER_IN1);
                           GPIOPinWrite(GPIO_PORTB, HEATER_IN2, 0);
                           GPIOPinWrite(GPIO_PORTB, HEATER2_IN3, HEATER2_IN3);
                           GPIOPinWrite(GPIO_PORTB, HEATER2_IN4, 0);
                           SetPWM_Duty1(70.0f);

                           if(temperature2 >= SETPOINT2) {
                                      //Sogutma ac
                               GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);
                               float output2 = Fuzzy_Cooler_Output(error2);
                               SetPWM_Duty2(output2);
                                   }
                           else {
                                       //Sogutma kapa
                               SetPWM_Duty2(0);
                               GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, 0);
                                   }


                       }
                       else{

                           if(temperature2 >= SETPOINT2) {
                               //Sogutma ac
                                                              GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, R_EN_PIN | L_EN_PIN);
                                                              float output2 = Fuzzy_Cooler_Output(error2);
                                                              SetPWM_Duty2(output2);

                                                              //Isýtma kapa
                                                            SetPWM_Duty1(0);
                                                            GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2, 0);
                                                            GPIOPinWrite(GPIO_PORTB, HEATER2_IN3 | HEATER2_IN4, 0);

                           }

                           else {
                               //Sogutma kapa
                                                           SetPWM_Duty2(0);
                                                           GPIOPinWrite(GPIO_PORTC_BASE, R_EN_PIN | L_EN_PIN, 0);
                                                           //Isýtma kapa
                                                           SetPWM_Duty1(0);
                                                           GPIOPinWrite(GPIO_PORTB, HEATER_IN1 | HEATER_IN2, 0);
                                                           GPIOPinWrite(GPIO_PORTB, HEATER2_IN3 | HEATER2_IN4, 0);
                           }

                       }
        }
        DelayMs(275);
    }
}
