/* USER CODE BEGIN Header */
/**
  **********
  * @file           : main.c
  * @brief          : Main program body
  **********
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  **********
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver.h"
#include "ring_buffer.h"
#include "keypad_driver.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- CONFIGURACION DEL SISTEMA ---
#define PASSWORD "123A" // Contraseña de 4 dígitos
#define PASSWORD_LEN 4
#define DEBOUNCE_TIME_MS 200      // Tiempo de anti-rebote para las teclas 
#define FEEDBACK_LED_TIME_MS 100  // Tiempo que el LED se enciende al oprimir cualquier tecla
#define SUCCESS_LED_TIME_MS 4000  // Tiempo que el LED se enciende cuando se ingresa la contraseña correcta
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// --- HANDLES Y BUFFERS ---
led_handle_t led1 = { .port = GPIOA, .pin = GPIO_PIN_5 }; // LD2 

keypad_handle_t keypad = {
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port},
    .row_pins  = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin},
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port},
    .col_pins  = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin}
};

#define KEYPAD_BUFFER_LEN 16
uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
ring_buffer_t keypad_rb;

char entered_password[PASSWORD_LEN + 1] = {0};
uint8_t password_index = 0;

// --- VARIABLES DE ESTADO PARA LOGICA NO BLOQUEANTE ---
uint32_t last_key_press_time = 0;
uint32_t led_timer_start = 0;
uint32_t led_on_duration = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void manage_led_timer(void);
void process_key(uint8_t key);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Callback de la interrupción externa GPIO.
  * @note   Esta función se llama cuando se detecta un flanco en un pin configurado para interrupción.
  *         Se mantiene muy rápida: solo lee el teclado y guarda la tecla en un buffer.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    char key = keypad_scan(&keypad, GPIO_Pin);
    if (key != '\0') {
        ring_buffer_write(&keypad_rb, (uint8_t)key);
    }
}

/**
 * @brief Procesa una tecla recibida del buffer del keypad.
 * @note  Contiene la lógica principal de la aplicación: feedback visual,
 *        almacenamiento de la contraseña y verificación.
 * @param key La tecla presionada a procesar.
 */
void process_key(uint8_t key)
{
    // 1. Proporcionar feedback visual inmediato al usuario
    led_on(&led1);
    led_timer_start = HAL_GetTick();
    led_on_duration = FEEDBACK_LED_TIME_MS;

    // 2. Almacenar el dígito si la contraseña no está completa
    if (password_index < PASSWORD_LEN) {
        entered_password[password_index++] = (char)key;
        printf("Digito presionado: %c\r\n", key);
    }

    // 3. Si la contraseña se ha completado, verificarla
    if (password_index == PASSWORD_LEN) {
        if (strncmp(entered_password, PASSWORD, PASSWORD_LEN) == 0) {
            printf("Contraseña correcta. ACCESO AUTORIZADO.\r\n");
            // Iniciar el temporizador largo para el LED de éxito
            led_on(&led1);
            led_timer_start = HAL_GetTick();
            led_on_duration = SUCCESS_LED_TIME_MS;
        } else {
            printf("Contraseña incorrecta. ACCESO DENEGADO: .\r\n");
            // Apagar el LED inmediatamente (o después del breve feedback)
            led_off(&led1);
            led_timer_start = 0; // Detener cualquier temporizador activo
        }
        
        // 4. Reiniciar para el siguiente intento
        password_index = 0;
        memset(entered_password, 0, sizeof(entered_password));
        printf("\nSistema de acceso listo. Ingrese la contraseña de 4 digitos...\r\n");
    }
}

/**
 * @brief Gestiona el apagado automático del LED sin bloquear el programa.
 * @note  Esta función debe ser llamada repetidamente en el bucle principal.
 */
void manage_led_timer(void)
{
    // Si el temporizador del LED está activo...
    if (led_timer_start != 0) {
        // ...y ha transcurrido el tiempo de duración...
        if (HAL_GetTick() - led_timer_start > led_on_duration) {
            // ...apagar el LED y desactivar el temporizador.
            led_off(&led1);
            led_timer_start = 0;
        }
    }
}

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // Inicialización de los drivers personalizados
  led_init(&led1);
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN);
  keypad_init(&keypad); // Asegura que las filas del keypad estén en BAJO

  printf("Sistema de Control de Acceso Iniciado.\r\n");
  printf("Ingrese la contraseña de 4 digitos...\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t key_from_buffer;
    
    // 1. Intentar leer una tecla del buffer (que es llenado por la interrupción)
    if (ring_buffer_read(&keypad_rb, &key_from_buffer)) {
        uint32_t current_time = HAL_GetTick();
        
        // 2. Aplicar lógica de anti-rebote (debounce) para evitar lecturas falsas
        if (current_time - last_key_press_time > DEBOUNCE_TIME_MS) {
            last_key_press_time = current_time; // Actualizar el tiempo de la última pulsación
            process_key(key_from_buffer);       // Procesar la tecla si el tiempo es válido
        }
    }

    // 3. Gestionar el temporizador del LED en cada iteración del bucle.
    //    Esto permite que el LED se apague solo sin detener el programa.
    manage_led_timer();
    
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  #ifdef GNUC
    setvbuf(stdout, NULL, _IONBF, 0);
  #endif
  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|KEYPAD_R1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, KEYPAD_R2_Pin|KEYPAD_R4_Pin|KEYPAD_R3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin KEYPAD_R1_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|KEYPAD_R1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : KEYPAD_C1_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEYPAD_C4_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KEYPAD_C2_Pin KEYPAD_C3_Pin */
  GPIO_InitStruct.Pin = KEYPAD_C2_Pin|KEYPAD_C3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : KEYPAD_R2_Pin KEYPAD_R4_Pin KEYPAD_R3_Pin */
  GPIO_InitStruct.Pin = KEYPAD_R2_Pin|KEYPAD_R4_Pin|KEYPAD_R3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  /* MODIFICADO: Se cambia la prioridad de 0 (máxima) a 5 (media-baja).
   * Un valor numérico más bajo significa una prioridad más alta en ARM Cortex-M.
   * Usar prioridad 0 es arriesgado, ya que puede bloquear interrupciones del sistema.
   * Una prioridad de 5 es segura para periféricos de usuario como un teclado.
  */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Sobrescribir la función _write para redirigir printf a la UART
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
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