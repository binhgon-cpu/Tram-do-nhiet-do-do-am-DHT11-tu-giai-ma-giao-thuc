/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (Đã thêm phần mở rộng DS18B20 PA4)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "dht11.h"
#include "i2c-lcd.h"
#include <stdio.h> // e dng hm sprintf ghp chuoi hien thi
#include "dht11_warning.h"
#include "ds18b20.h" // <<< [THÊM MỚI] Include driver DS18B20

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// Tao mang 8 byte ve bieu tuong ( C) cho CGRAM
uint8_t Custom_Degree[8] = {
    0x06,
    0x09, 
    0x09, 
    0x06, 
    0x00,
    0x00,
    0x00,
    0x00
};

uint8_t nhiet_do = 0, do_am = 0;
float ds18b20_temp = 0.0f; // <<< [THÊM MỚI] Biến lưu giá trị nhiệt độ DS18B20

uint8_t temp_min = 99, temp_max = 0;
uint8_t hum_min = 99, hum_max = 0;

uint32_t last_page_change = 0;
uint8_t display_page = 0; // 0: Trang so sánh DHT11 vs DS18B20, 1: Trang Min/Max
char lcd_buff[20];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);

  lcd_init();
  lcd_clear();
  lcd_create_custom_char(0, Custom_Degree);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      //Quét nút bấm tăng/giảm ngưỡng cài đặt (PA2 & PA3)
      Scan_Buttons();
		
      //Đọc giá trị từ cảm biến DS18B20 (Chân PA4) mỗi 800ms
      static uint32_t last_ds18b20_read = 0;
      if (HAL_GetTick() - last_ds18b20_read >= 800)
      {
          ds18b20_temp = DS18B20_ReadTemp();
          last_ds18b20_read = HAL_GetTick();
      }

      // 3. Đọc dữ liệu từ DHT11 (0 là THÀNH CÔNG)
      if (DHT11_Read(&nhiet_do, &do_am) == 0) 
      { 
          // Cập nhật giá trị Min / Max
          if (nhiet_do < temp_min) temp_min = nhiet_do;
          if (nhiet_do > temp_max) temp_max = nhiet_do;
          if (do_am < hum_min) hum_min = do_am;
          if (do_am > hum_max) hum_max = do_am;

          // --- XỬ LÝ CẢNH BÁO LED RGB VÀ CÒI (PA12) ---
          if (nhiet_do >= threshold_temp) 
          { 
              // Quá ngưỡng: Còi kêu + LED RGB Đỏ sáng
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET); 
              Set_RGB_Color(65535, 0, 0); // Đỏ
          } 
          else if (nhiet_do >= (threshold_temp - 2))
          {
              // Sát ngưỡng: Tắt còi + LED RGB Vàng
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
              Set_RGB_Color(63500, 63500, 0); // Vàng
          }
          else 
          {
              // Bình thường: Tắt còi + LED RGB Xanh lá
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
              Set_RGB_Color(0, 65535, 0); // Xanh lá
          }

          // --- UART LOG ĐỊNH KỲ (Bổ sung thêm cột DS18B20 dạng CSV) ---
          char uart_buff[80];
          // Định dạng: Nhiệt_Độ_DHT11, Độ_Ẩm, Nhiệt_Độ_DS18B20, Ngưỡng
          int len = sprintf(uart_buff, "%d,%d,%.2f,%d\r\n", nhiet_do, do_am, ds18b20_temp, threshold_temp);
          HAL_UART_Transmit(&huart1, (uint8_t*)uart_buff, len, 100);

          // 4. Chuyển trang màn hình LCD sau mỗi 5 GIÂY
          if (HAL_GetTick() - last_page_change > 5000) {
              display_page = !display_page; // Đổi trang 0 <-> 1
              last_page_change = HAL_GetTick();
              lcd_clear();
          }

          // --- Trang 0: Hiển thị So sánh Nhiệt độ DHT11, DS18B20 & Độ ẩm ---
          if (display_page == 0) {
              // Dòng 1: DHT:XX C DS:XX.X C
              lcd_put_cur(0, 0);
              sprintf(lcd_buff, "DHT:%2d", nhiet_do);
              lcd_send_string(lcd_buff);
              lcd_send_data(0); // Ký tự °C
              sprintf(lcd_buff, "C DS:%.1f", ds18b20_temp);
              lcd_send_string(lcd_buff);
              lcd_send_data(0);

              // Dòng 2: H:XX% Set:YY C
              lcd_put_cur(1, 0);
              sprintf(lcd_buff, "H:%2d%% Set:%2d", do_am, threshold_temp);
              lcd_send_string(lcd_buff);
              lcd_send_data(0);
              lcd_send_string("C ");
          } 
          // --- Trang 1: Hiển thị giá trị MIN / MAX ---
          else {
              lcd_put_cur(0, 0);
              sprintf(lcd_buff, "T: %2d - %2d", temp_min, temp_max);
              lcd_send_string(lcd_buff);
              lcd_send_data(0);
              lcd_send_string("C  ");

              lcd_put_cur(1, 0);
              sprintf(lcd_buff, "H: %2d - %2d %%   ", hum_min, hum_max);
              lcd_send_string(lcd_buff);
          }

      } 
      else 
      {
          // Báo lỗi cảm biến khi DHT11_Read trả về khác 0
          lcd_put_cur(0, 0);
          lcd_send_string("LOI CAM BIEN!    ");
          lcd_put_cur(1, 0);
          lcd_send_string("KIEM TRA DAY... ");
      }

      HAL_Delay(200); // Trễ 200ms để nút bấm phản hồi nhạy

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 70;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET);

  /* Configure GPIO pin Output Level cho DS18B20 PA4 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // <<< Đặt chân PA4 mặc định ở mức cao (HIGH)

  /*Configure GPIO pins : PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 (DS18B20 Data Pin) */
  GPIO_InitStruct.Pin = GPIO_PIN_4; // <<< [THÊM MỚI] Cài đặt chân PA4 làm chân DS18B20
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_PIN_Pin */
  GPIO_InitStruct.Pin = DHT11_PIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_PIN_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif