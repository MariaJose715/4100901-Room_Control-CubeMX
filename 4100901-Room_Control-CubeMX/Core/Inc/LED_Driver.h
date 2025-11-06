#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>
#include "main.h"

// Tipos de datos públicos
typedef struct {
    GPIO_TypeDef *port;  // Puerto del LED (por ejemplo, GPIOA)
    uint16_t pin;        // Pin del LED (por ejemplo, GPIO_PIN_5)
} led_handle_t;

// API pública
/**
 * @brief Inicializa el LED configurando el pin como salida.
 */
void led_init(led_handle_t *led);
 /**
  * @brief Enciende el LED.
  */
void led_on(led_handle_t *led);
/**
 * @brief Apaga el LED.
 */ 
void led_off(led_handle_t *led);
/**
 * @brief Cambia el estado del LED (enciende si está apagado y viceversa).
 */
void led_toggle(led_handle_t *led);


#endif // LED_DRIVER_H


