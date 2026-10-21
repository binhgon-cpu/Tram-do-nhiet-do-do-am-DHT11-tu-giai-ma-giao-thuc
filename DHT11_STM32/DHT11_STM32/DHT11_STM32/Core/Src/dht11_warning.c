#include "dht11_warning.h"

// G?i Timer 3 t? main.c cho PWM LED RGB
extern TIM_HandleTypeDef htim3; 

// Kh?i t?o ngu?ng nhi?t d? m?c d?nh (35°C)
uint8_t threshold_temp = 35; 

// 1. Hàm di?u khi?n d? sáng PWM cho LED RGB (TIM3 Kênh 1, 2, 3)
void Set_RGB_Color(uint16_t red, uint16_t green, uint16_t yellow)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, red);   // PA6 (Red)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, green); // PA7 (Green)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, yellow);  // PB0 (Blue)
}

// 2. Hàm quét và ch?ng rung 2 nút b?m Tang/Gi?m ngu?ng (PA2, PA3)
void Scan_Buttons(void)
{
    // Nút Tang ngu?ng (PA2)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
        HAL_Delay(20); // Ch?ng rung nút b?m
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
            threshold_temp++;
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET); // Ch? th? nút
        }
    }

    // Nút Gi?m ngu?ng (PA3)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
            threshold_temp--;
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET); // Ch? th? nút
        }
    }
}