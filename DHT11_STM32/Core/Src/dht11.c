#include "dht11.h"

// Gọi Timer 2 từ main.c
extern TIM_HandleTypeDef htim2;

// Hàm tạo trễ bằng us
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

//Hàm chuyển chân GPIO thành Output (Để chip truyền lệnh đi)
void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

//Hàm chuyển chân GPIO thành Input (Để chip lắng nghe cảm biến)
void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// HÀM ĐỌC CẢM BIẾN CHÍNH
uint8_t DHT11_Read(uint8_t *nhiet_do, uint8_t *do_am) {
    uint8_t data[5] = {0, 0, 0, 0, 0}; // Mảng chứa 5 byte dữ liệu
    uint16_t thoi_gian_muc_cao = 0;

    //ĐÁNH THỨC CẢM BIẾN (START SIGNAL)
    Set_Pin_Output(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin);

    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, 0);
    delay_us(18000);

    HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, 1);
    delay_us(20);

    Set_Pin_Input(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin);        // Chuyển sang chế độ nghe

    //CHỜ DHT11 PHẢN HỒI
    //Nếu quá 100us mà trạng thái không đổi -> Báo lỗi Timeout (1)

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == 1) {
        if (__HAL_TIM_GET_COUNTER(&htim2) > 100) return 1;
    }

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == 0) {
        if (__HAL_TIM_GET_COUNTER(&htim2) > 100) return 1;
    }

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == 1) {
        if (__HAL_TIM_GET_COUNTER(&htim2) > 100) return 1;
    }

    //ĐỌC 40 BIT DỮ LIỆU

    for (int i = 0; i < 5; i++) { // Đọc 5 byte
        for (int j = 0; j < 8; j++) { // Đọc 8 bit của từng byte

            // Chờ qua mức 0 ban đầu của mỗi bit
            __HAL_TIM_SET_COUNTER(&htim2, 0);
            while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == 0) {
                if (__HAL_TIM_GET_COUNTER(&htim2) > 100) return 1;
            }

            // Bắt đầu đo thời gian tồn tại của mức 1
            __HAL_TIM_SET_COUNTER(&htim2, 0);
            while (HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin) == 1) {
                if (__HAL_TIM_GET_COUNTER(&htim2) > 100) return 1;
            }
            thoi_gian_muc_cao = __HAL_TIM_GET_COUNTER(&htim2);

            // Xử lý dữ liệu: Dịch trái 1 bit
            data[i] <<= 1;

            // Nếu mức 1 tồn tại lâu hơn 40us, nó là bit 1. Nếu không, nó là bit 0
            if (thoi_gian_muc_cao > 40) {
                data[i] |= 1;
            }
        }
    }

    // BƯỚC 4: KIỂM TRA TÍNH TOÀN VẸN (CHECKSUM)

    //data[0] = data[0] + 1;
    if (data[0] + data[1] + data[2] + data[3] == data[4]) {
        *do_am = data[0];
        *nhiet_do = data[2];
        return 0; // Số 0 là thành công
    } else {
        return 2; // Số 2 là lỗi sai dữ liệu
    }
}
