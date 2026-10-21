#include "ds18b20.h"

// Tham chi?u d?n TIM2 dã du?c kh?i t?o trong main.c (Prescaler = 71 -> 1 tick = 1us)
extern TIM_HandleTypeDef htim2;

/**
  * @brief Hàm t?o tr? microgiây s? d?ng Timer 2
  * @param us: S? microgiây c?n tr?
  */
static void delay_us(uint16_t us) 
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

/**
  * @brief C?u hình chân PA4 làm Chân Ra (Output Push-Pull)
  */
static void Set_Pin_Output(void) 
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS18B20_PIN_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief C?u hình chân PA4 làm Chân Vào (Input Floating / Pull-up)
  */
static void Set_Pin_Input(void) 
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_PIN_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief G?i xung Reset kh?i d?ng giao ti?p 1-Wire
  * @retval 1: Có ph?n h?i t? DS18B20 (Presence), 0: Không có c?m bi?n
  */
static uint8_t DS18B20_Start(void) 
{
    uint8_t Response = 0;
    
    // 1. Kéo chân PA4 xu?ng LOW ít nh?t 480us
    Set_Pin_Output();
    HAL_GPIO_WritePin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin, GPIO_PIN_RESET);
    delay_us(480);

    // 2. Nh? chân PA4 lên HIGH và ch? 80us
    Set_Pin_Input();
    delay_us(80);

    // 3. Ð?c m?c di?n áp trên bus (N?u c?m bi?n ho?t d?ng t?t, nó s? kéo chân xu?ng LOW)
    if (!(HAL_GPIO_ReadPin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin))) 
    {
        Response = 1; // Có c?m bi?n ph?n h?i
    } 
    else 
    {
        Response = 0; // Không th?y c?m bi?n
    }

    // Ch? h?t chu k? Reset (400us còn l?i)
    delay_us(400);
    return Response;
}

/**
  * @brief Ghi 1 byte d? li?u sang DS18B20
  * @param data: Byte c?n ghi
  */
static void DS18B20_Write(uint8_t data) 
{
    for (int i = 0; i < 8; i++) 
    {
        if ((data & (1 << i)) != 0) 
        {
            // Ghi bit 1
            Set_Pin_Output();
            HAL_GPIO_WritePin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin, GPIO_PIN_RESET);
            delay_us(2);
            Set_Pin_Input();
            delay_us(60);
        } 
        else 
        {
            // Ghi bit 0
            Set_Pin_Output();
            HAL_GPIO_WritePin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin, GPIO_PIN_RESET);
            delay_us(60);
            Set_Pin_Input();
            delay_us(2); // Thêm th?i gian ph?c h?i (Recovery time >= 1us) gi?a các khe ghi
        }
    }
}

static uint8_t DS18B20_Read(void) 
{
    uint8_t value = 0;

    for (int i = 0; i < 8; i++) 
    {
        Set_Pin_Output();
        HAL_GPIO_WritePin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin, GPIO_PIN_RESET);
        delay_us(2);

        Set_Pin_Input();
        delay_us(10); // Ch? 10us cho su?n lên ?n d?nh tru?c khi l?y m?u (t?ng < 15us)
        if (HAL_GPIO_ReadPin(DS18B20_PIN_GPIO_Port, DS18B20_PIN_Pin)) 
        {
            value |= (1 << i);
        }
        delay_us(50); // Ch? h?t slot 60us
    }
    return value;
}

float DS18B20_ReadTemp(void) 
{
    uint8_t temp_lsb, temp_msb;
    int16_t temp;
    float temperature = -99.0f;

    // 1. Ð?c k?t qu? dã chuy?n d?i xong t? Scratchpad
    if (DS18B20_Start() == 1)
    {
        DS18B20_Write(0xCC); // Skip ROM
        DS18B20_Write(0xBE); // Read Scratchpad

        temp_lsb = DS18B20_Read();
        temp_msb = DS18B20_Read();

        temp = (int16_t)((temp_msb << 8) | temp_lsb);
        temperature = (float)temp / 16.0f;
    }

    // 2. Phát l?nh Convert T m?i d? c?m bi?n t? do ng?m cho l?n g?i sau
    if (DS18B20_Start() == 1) 
    {
        DS18B20_Write(0xCC); // Skip ROM
        DS18B20_Write(0x44); // Convert T (Không g?i DS18B20_Start() sau l?nh này)
    }

    return temperature;
}