#ifndef LCD_I2C_H_
#define LCD_I2C_H_

#include <stdint.h>

void I2C1_Init(void);
void LCD_Init(void);
void LCD_SendCommand(unsigned char cmd);
void LCD_SendData(unsigned char data);
void LCD_SendString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_WriteAt(uint8_t row, uint8_t col,  char* str);
void LCD_Clear(void);

#endif /* LCD_I2C_H_ */
