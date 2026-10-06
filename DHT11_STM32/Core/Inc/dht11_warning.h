#ifndef INC_DHT11_WARNING_H_
#define INC_DHT11_WARNING_H_

#include "main.h"

// Bi?n toàn c?c khai báo extern d? main.c ho?c các module khác (nhu LCD) có th? d?c
extern uint8_t threshold_temp;
extern uint8_t current_temp;
extern uint8_t current_hum;

// Khai báo các hàm ch?c nang chính
void delay_us(uint16_t us);
void DHT11_Set_Pin_Output(void);
void DHT11_Set_Pin_Input(void);
uint8_t DHT11_Read(uint8_t *temp, uint8_t *hum);
void Set_RGB_Color(uint16_t red, uint16_t green, uint16_t blue);
void Scan_Buttons(void);
void Warning_Process(void);

#endif /* INC_DHT11_WARNING_H_ */