#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

// Ð?nh nghia t? d?ng theo tên nhãn "DS18B20_PIN" d?t trên CubeMX cho chân PA4
#ifndef DS18B20_PIN_Pin
#define DS18B20_PIN_Pin       GPIO_PIN_4
#define DS18B20_PIN_GPIO_Port GPIOA
#endif

float DS18B20_ReadTemp(void);

#endif /* __DS18B20_H */