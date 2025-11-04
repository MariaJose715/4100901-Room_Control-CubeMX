#include "led_driver.h"
#include "main.h"

/**
 * @brief Inicializa el LED apagándolo (estado bajo).
 */
void led_init(led_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
}

/**
 * @brief Enciende el LED (estado alto).
 */
void led_on(led_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
}

/**
 * @brief Apaga el LED (estado bajo).
 */
void led_off(led_handle_t *led) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
}

/**
 * @brief Alterna el estado del LED (si está encendido lo apaga y viceversa).
 */
void led_toggle(led_handle_t *led) {
    HAL_GPIO_TogglePin(led->port, led->pin);
}
