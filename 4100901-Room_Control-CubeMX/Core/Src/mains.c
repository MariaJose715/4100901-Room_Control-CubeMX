/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Programa principal del sistema de control de acceso
  ******************************************************************************
  * @attention
  * 
  * Este proyecto integra tres librerías modulares:
  * - led_driver: manejo del LED mediante GPIO.
  * - ring_buffer: buffer circular para manejo asíncrono de entradas.
  * - keypad_driver: driver de teclado matricial 4x4 con interrupciones.
  * 
  * Su función es permitir el ingreso de una contraseña mediante el teclado.
  * Si la clave es correcta, el LED se enciende por unos segundos.
  * 
  ******************************************************************************
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

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PASSWORD "123A"              // Contraseña esperada
#define PASSWORD_LEN 4
#define DEBOUNCE_TIME_MS 200         // Tiempo de antirrebote entre teclas
#define FEEDBACK_LED_TIME_MS 100     // Tiempo que el LED se enciende por pulsación
#define SUCCESS_LED_TIME_MS 4000     // Tiempo de encendido tras contraseña correcta
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// --- LED principal (LD2 en la Nucleo) ---
led_handle_t led1 = { .port = GPIOA, .pin = GPIO_PIN_5 };

// --- Teclado 4x4 ---
keypad_handle_t keypad = {
    .row_ports = {KEYPAD_R1_GPIO_Port, KEYPAD_R2_GPIO_Port, KEYPAD_R3_GPIO_Port, KEYPAD_R4_GPIO_Port},
    .row_pins  = {KEYPAD_R1_Pin, KEYPAD_R2_Pin, KEYPAD_R3_Pin, KEYPAD_R4_Pin},
    .col_ports = {KEYPAD_C1_GPIO_Port, KEYPAD_C2_GPIO_Port, KEYPAD_C3_GPIO_Port, KEYPAD_C4_GPIO_Port},
    .col_pins  = {KEYPAD_C1_Pin, KEYPAD_C2_Pin, KEYPAD_C3_Pin, KEYPAD_C4_Pin}
};

// --- Buffer circular para teclas ---
#define KEYPAD_BUFFER_LEN 16
uint8_t keypad_buffer[KEYPAD_BUFFER_LEN];
ring_buffer_t keypad_rb;

// --- Variables de control de acceso ---
char entered_password[PASSWORD_LEN + 1] = {0};
uint8_t password_index = 0;

// --- Variables de temporización ---
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

/* USER CODE BEGIN 0 */

/**
  * @brief  Callback de la interrupción externa (EXTI).
  * @note   Se ejecuta al detectar un flanco descendente en una columna del teclado.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    char key = keypad_scan(&keypad, GPIO_Pin);
    if (key != '\0') {
        ring_buffer_write(&keypad_rb, (uint8_t)key);
    }
}

/**
 * @brief Procesa una tecla recibida del buffer del teclado.
 * @note  Se encarga del feedback visual y la validación de la contraseña.
 */
void process_key(uint8_t key)
{
    // Encender LED brevemente como feedback
    led_on(&led1);
    led_timer_start = HAL_GetTick();
    led_on_duration = FEEDBACK_LED_TIME_MS;

    // Guardar el dígito ingresado
    if (password_index < PASSWORD_LEN) {
        entered_password[password_index++] = (char)key;
        printf("Tecla presionada: %c\r\n", key);
    }

    // Si ya se ingresaron 4 dígitos, verificar contraseña
    if (password_index == PASSWORD_LEN) {
        if (strncmp(entered_password, PASSWORD, PASSWORD_LEN) == 0) {
            printf("Contraseña correcta. ACCESO AUTORIZADO.\r\n");
            led_on(&led1);
            led_timer_start = HAL_GetTick();
            led_on_duration = SUCCESS_LED_TIME_MS;
        } else {
            printf("Contraseña incorrecta. ACCESO DENEGADO.\r\n");
            led_off(&led1);
        }

        // Reiniciar el ingreso
        password_index = 0;
        memset(entered_password, 0, sizeof(entered_password));
        printf("\nIngrese la contraseña de 4 dígitos...\r\n");
    }
}

/**
 * @brief  Apaga el LED automáticamente cuando el tiempo indicado termina.
 */
void manage_led_timer(void)
{
    if (led_timer_start != 0 && (HAL_GetTick() - led_timer_start > led_on_duration)) {
        led_off(&led1);
        led_timer_start = 0;
    }
}

/* USER CODE END 0 */

/**
  * @brief  Punto de entrada principal.
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  led_init(&led1);
  keypad_init(&keypad);
  ring_buffer_init(&keypad_rb, keypad_buffer, KEYPAD_BUFFER_LEN);

  printf("\r\nSistema de Control de Acceso Iniciado.\r\n");
  printf("Ingrese la contraseña de 4 dígitos...\r\n");
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t key_from_buffer;

    // Leer teclas del buffer circular
    if (ring_buffer_read(&keypad_rb, &key_from_buffer)) {
        uint32_t now = HAL_GetTick();
        if (now - last_key_press_time > DEBOUNCE_TIME_MS) {
            last_key_press_time = now;
            process_key(key_from_buffer);
        }
    }

    // Manejo del temporizador del LED (no bloqueante)
    manage_led_timer();
  }
  /* USER CODE END WHILE */
}

/* ---------------------- FUNCIONES DE HARDWARE ---------------------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    Error_Handler();

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
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
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
    Error_Handler();

  #ifdef GNUC
    setvbuf(stdout, NULL, _IONBF, 0);
  #endif
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Salidas: LED + Filas del teclado */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|KEYPAD_R1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, KEYPAD_R2_Pin|KEYPAD_R3_Pin|KEYPAD_R4_Pin, GPIO_PIN_RESET);

  /* Entradas con interrupción: columnas */
  GPIO_InitStruct.Pin = KEYPAD_C1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C1_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KEYPAD_C2_Pin|KEYPAD_C3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KEYPAD_C4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_C4_GPIO_Port, &GPIO_InitStruct);

  /* Configurar filas como salida */
  GPIO_InitStruct.Pin = KEYPAD_R1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KEYPAD_R2_Pin|KEYPAD_R3_Pin|KEYPAD_R4_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* LED interno LD2 */
  GPIO_InitStruct.Pin = LD2_Pin;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Interrupciones */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* Redirección printf a UART */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
