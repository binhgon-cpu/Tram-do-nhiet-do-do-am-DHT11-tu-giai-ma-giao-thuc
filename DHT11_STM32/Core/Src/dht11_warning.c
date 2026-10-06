#include "dht11_warning.h"

// Khai báo các b? Timer do STM32CubeMX t? d?ng sinh ra trong main.c
extern TIM_HandleTypeDef htim2; // Timer d?m microsecond (1us)
extern TIM_HandleTypeDef htim3; // Timer PWM cho LED RGB

// Kh?i t?o giá tr? ban d?u cho các bi?n
uint8_t threshold_temp = 35; // Ngu?ng nhi?t d? m?c d?nh (35°C)
uint8_t current_temp = 0;
uint8_t current_hum = 0;

// 1. Hàm delay microsecond b?ng Timer2
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// 2. Chuy?n chân PB5 (DHT11) sang Output Push-Pull
void DHT11_Set_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);
}

// 3. Chuy?n chân PB5 (DHT11) sang Input Pull-up
void DHT11_Set_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);
}

// 4. Hàm t? gi?i mã giao th?c 1-wire c?a DHT11
uint8_t DHT11_Read(uint8_t *temp, uint8_t *hum)
{
    uint8_t data[5] = {0};
    uint16_t timeout = 0;

    // Phase 1: G?i tín hi?u Start (LOW 18ms -> HIGH 30us)
    DHT11_Set_Pin_Output();
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET);
    delay_us(30);

    // Phase 2: Nh?n tín hi?u Ph?n h?i (Response) t? DHT11
    DHT11_Set_Pin_Input();

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        delay_us(1);
        if (++timeout > 100) return 0; // L?i timeout
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
        delay_us(1);
        if (++timeout > 100) return 0;
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
        delay_us(1);
        if (++timeout > 100) return 0;
    }

    // Phase 3: Ð?c và gi?i mã 40 bit d? li?u
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_RESET) {
            delay_us(1);
            if (++timeout > 100) return 0;
        }

        // Ðo d? r?ng xung HIGH b?ng Timer2
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == GPIO_PIN_SET) {
            if (__HAL_TIM_GET_COUNTER(&htim2) > 100) break;
        }
        uint16_t high_time = __HAL_TIM_GET_COUNTER(&htim2);

        // Phân bi?t bit 0 (26-28us) và bit 1 (70us)
        if (high_time > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Phase 4: Ki?m tra Checksum
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *hum = data[0];
        *temp = data[2];
        return 1; // Ð?c thành công
    }
    return 0; // L?i Checksum
}

// 5. Hàm di?u khi?n d? sáng PWM cho LED RGB (TIM3 Kênh 1, 2, 3)
void Set_RGB_Color(uint16_t red, uint16_t green, uint16_t blue)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, red);   // PA6 (Red)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, green); // PA7 (Green)
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, blue);  // PB0 (Blue)
}

// 6. Hàm quét và ch?ng rung 2 nút b?m Tang/Gi?m ngu?ng (PA2, PA3)
void Scan_Buttons(void)
{
    // Nút Tang ngu?ng (PA2)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
        HAL_Delay(20); // Anti-bounce (Ch?ng rung nút)
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

// 7. Hàm t?ng h?p x? lý toàn b? logic C?nh báo
void Warning_Process(void)
{
    // Quét tr?ng thái nút b?m
    Scan_Buttons();

    // Ð?c d? li?u t? DHT11
    if (DHT11_Read(&current_temp, &current_hum))
    {
        // So sánh v?i ngu?ng cài d?t
        if (current_temp >= threshold_temp)
        {
            // VU?T NGU?NG: LED RGB màu Ð? + B?t Còi Buzzer (PB1)
            Set_RGB_Color(65535, 0, 0);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); 
        }
        else if (current_temp >= (threshold_temp - 2))
        {
            // SÁT NGU?NG: LED RGB màu Vàng + T?t Còi
            Set_RGB_Color(65535, 20000, 0);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        }
        else
        {
            // AN TOÀN: LED RGB màu Xanh lá + T?t Còi
            Set_RGB_Color(0, 65535, 0);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        }
    }
}