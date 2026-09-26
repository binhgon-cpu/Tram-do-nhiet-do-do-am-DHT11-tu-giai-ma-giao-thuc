#include "dht11.h"

extern TIM_HandleTypeDef htim2; // goi timer 2 tu main.c

// 1. Ham delay micro giay
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// 2. Ham chuyen chan GPIO thanh Output
void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// 3. Ham chuyen chan GPIO thanh Input
void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// 4. Ham đoc va giai ma DHT11
uint8_t DHT11_Read(uint8_t *nhiet_do, uint8_t *do_am) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint16_t timeout_counter = 0;

    // Bước 1: MCU gui tin hieu Start (Keo xuong 0 trong 18ms)
    Set_Pin_Output(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    delay_us(30);

    // Chuyen sang đoc tin hieu
    Set_Pin_Input(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin);

    // Buoc 2: Cho DHT11 phan hoi keo xuong 0
    timeout_counter = 0;
    while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        delay_us(1);
        if(++timeout_counter > 100) return 1; // ERR_TIMEOUT
    }

    // Cho DHT11 keo len 1
    timeout_counter = 0;
    while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
        delay_us(1);
        if(++timeout_counter > 100) return 1;
    }

    // Cho DHT11 keo xuong 0 lai đe bat đau gui data
    timeout_counter = 0;
    while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        delay_us(1);
        if(++timeout_counter > 100) return 1;
    }

    // Buoc 3: doc 40 bit
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            // doi het muc 0
            timeout_counter = 0;
            while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
                delay_us(1);
                if(++timeout_counter > 100) return 1;
            }

            // Cho 40us de do do dai muc 1
            delay_us(40);

            if (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                data[i] |= (1 << j); // Luu bit 1
                // Cho den khi het muc 1
                timeout_counter = 0;
                while(HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
                    delay_us(1);
                    if(++timeout_counter > 100) return 1;
                }
            }
        }
    }

    // Buoc 4: Kiem tra Checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return 2; // ERR_CHECKSUM
    }

    *do_am = data[0];
    *nhiet_do = data[2];
    return 0; // OK
}
