#include "inc/tm4c123gh6pm.h"
#include "LCD_I2C.h"
#include "stdint.h"
#include "DelayMs.h"

#define I2C_MCS_RUN      0x00000001
#define I2C_MCS_START    0x00000002
#define I2C_MCS_STOP     0x00000004
#define I2C_MCS_ACK      0x00000008
#define I2C_MCS_ERROR    0x00000002

#define SLAVE_ADDRESS 0x27 // LCD I2C modülü adresi (genelde 0x27 veya 0x3F olur)
#define ENABLE 0x04
#define RW 0x02
#define LCD_BACKLIGHT 0x08
#define RS 0x01

// I2C1 initialization, PA6 -> SCL, PA7 -> SDA
void I2C1_Init(void){
    SYSCTL_RCGCI2C_R |= (1 << 1);   // I2C1 clock enable
    SYSCTL_RCGCGPIO_R |= (1 << 0);  // Port A clock enable
    while((SYSCTL_PRGPIO_R & (1 << 0)) == 0){} // Wait for Port A ready

    GPIO_PORTA_AFSEL_R |= (1 << 6) | (1 << 7);    // PA6, PA7 Alternate function
    GPIO_PORTA_ODR_R |= (1 << 7);                 // PA7 (SDA) open-drain
    GPIO_PORTA_DEN_R |= (1 << 6) | (1 << 7);       // Digital enable PA6, PA7
    GPIO_PORTA_PCTL_R &= ~((0xF << 24) | (0xF << 28)); // Clear
    GPIO_PORTA_PCTL_R |= (3 << 24) | (3 << 28);    // Configure PA6, PA7 for I2C1

    I2C1_MCR_R = 0x10;             // Master mode
    I2C1_MTPR_R = 7;               // 100kbps @ 16MHz
}

// I2C1 send one byte
void I2C1_SendByte(unsigned char slave_addr, unsigned char data){
    I2C1_MSA_R = (slave_addr << 1); // Write mode
    I2C1_MDR_R = data;
    I2C1_MCS_R = (I2C_MCS_START | I2C_MCS_STOP | I2C_MCS_RUN);
    while(I2C1_MCS_R & I2C_MCS_RUN){}
}

// LCD low-level function to send 4 bits
void LCD_SendNibble(unsigned char data){
    I2C1_SendByte(SLAVE_ADDRESS, data | LCD_BACKLIGHT | ENABLE);
    DelayMs(1);
    I2C1_SendByte(SLAVE_ADDRESS, (data | LCD_BACKLIGHT) & ~ENABLE);
}

// LCD send command
void LCD_SendCommand(unsigned char cmd){
    unsigned char upper = cmd & 0xF0;
    unsigned char lower = (cmd << 4) & 0xF0;
    LCD_SendNibble(upper);
    LCD_SendNibble(lower);
}

// LCD send data (character)
void LCD_SendData(unsigned char data){
    unsigned char upper = (data & 0xF0) | RS;
    unsigned char lower = ((data << 4) & 0xF0) | RS;
    LCD_SendNibble(upper);
    LCD_SendNibble(lower);
}

// LCD initialization sequence
void LCD_Init(void){
    DelayMs(50);
    LCD_SendNibble(0x30);
    DelayMs(5);
    LCD_SendNibble(0x30);
    DelayMs(5);
    LCD_SendNibble(0x30);
    DelayMs(5);
    LCD_SendNibble(0x20); // 4-bit mode
    DelayMs(5);

    LCD_SendCommand(0x28); // 4-bit, 2-line, 5x8 font
    LCD_SendCommand(0x08); // Display off
    LCD_SendCommand(0x01); // Clear display
    DelayMs(2);
    LCD_SendCommand(0x06); // Entry mode set
    LCD_SendCommand(0x0C); // Display on, cursor off
}

// LCD send string
void LCD_SendString(char *str){
    while(*str){
        LCD_SendData(*str++);
    }
}

// Clear LCD
void LCD_Clear(void){
    LCD_SendCommand(0x01); // Clear display
    DelayMs(2);

}


// LCD set cursor to a specific row and column
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address;

    switch (row) {
        case 0:
            address = 0x00 + col;
            break;
        case 1:
            address = 0x40 + col;
            break;
        default:
            address = 0x00 + col;
            break;
    }

    LCD_SendCommand(0x80 | address);
}

// LCD write at specific location
void LCD_WriteAt(uint8_t row, uint8_t col, char* str) {
    LCD_SetCursor(row, col);
    DelayMs(5);
    LCD_SendString(str);
    DelayMs(5);
}
