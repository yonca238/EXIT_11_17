/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "stdio.h"
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

/* USER CODE BEGIN PV */
static OLED_t g_oled;
static uint8_t display_dirty = 0U;
volatile int32_t encoder_count = 0;
volatile uint8_t encoder_changed = 0U;
volatile uint8_t button_clear_flag = 0U;
static uint8_t encoder_last_state = 0U;

volatile uint8_t scale_button_flag = 0U;
static uint8_t encoder_scale = 1U;       // 1�?2�?4 �?
static uint32_t last_scale_tick = 0U;
static int32_t encoder_last_detent = 0;
static int32_t display_count = 0;

static const int8_t encoder_table[16] =
{
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_6 || GPIO_Pin == GPIO_PIN_7)
  {
    uint8_t a = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
    uint8_t b = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
    uint8_t current_state = (uint8_t)((a << 1) | b);

    int8_t delta = encoder_table[
        (encoder_last_state << 2) | current_state
    ];

    encoder_last_state = current_state;

    if (delta != 0)
    {
      encoder_count += delta;
      encoder_changed = 1U;
    }
  }
  else if (GPIO_Pin == GPIO_PIN_2)
  {
    button_clear_flag = 1U;
  }
	else if (GPIO_Pin == GPIO_PIN_1)
  {
    scale_button_flag = 1U;
  }
}
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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	if (OLED_Init(&g_oled, &hi2c1) != 0U)
{
  OLED_Clear(&g_oled);

  OLED_ShowTextLarge(&g_oled, 0U, 20U, "COUNT READY");

  OLED_Update(&g_oled);
}
	encoder_last_state =
    (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) << 1) |
    HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
{
  char count_text[12];
  char title_text[16];

  /* PA2 清零 */
  if (button_clear_flag != 0U)
  {
    button_clear_flag = 0U;

    encoder_count = 0;
    encoder_last_detent = 0;
    display_count = 0;
    display_dirty = 1U;
  }

  /* PA1 切换倍率：1 -> 2 -> 4 -> 1 */
  if (scale_button_flag != 0U)
  {
    uint32_t now = HAL_GetTick();

    scale_button_flag = 0U;

    if ((now - last_scale_tick) >= 200U)
    {
      last_scale_tick = now;

      if (encoder_scale == 1U)
      {
        encoder_scale = 2U;
      }
      else if (encoder_scale == 2U)
      {
        encoder_scale = 4U;
      }
      else
      {
        encoder_scale = 1U;
      }

      display_dirty = 1U;
    }
  }

  /* 编码器四倍频原始计数转成格数 */
  if (encoder_changed != 0U)
  {
    int32_t detent_count;
    int32_t detent_delta;

    encoder_changed = 0U;

    detent_count = encoder_count / 4;
    detent_delta = detent_count - encoder_last_detent;

    if (detent_delta != 0)
    {
      display_count += detent_delta * encoder_scale;
      encoder_last_detent = detent_count;
      display_dirty = 1U;
    }
  }

  /* 只有状态变化时才刷新 OLED */
  if (display_dirty != 0U)
  {
    display_dirty = 0U;

    snprintf(count_text,
             sizeof(count_text),
             "%ld",
             (long)display_count);

    snprintf(title_text,
             sizeof(title_text),
             "COUNT x%u",
             (unsigned)encoder_scale);

    OLED_ShowTwoLines(&g_oled, title_text, count_text);
  }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
