#ifndef KEYPAD_DRIVER_H
#define KEYPAD_DRIVER_H

#include "main.h"
#include <stdint.h>

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

/**
 * @brief Estructura que contiene los puertos y pines del teclado.
 * @note  Esta estructura define las conexiones físicas del keypad.
 */
typedef struct {
    GPIO_TypeDef* row_ports[KEYPAD_ROWS];
    uint16_t row_pins[KEYPAD_ROWS];
    GPIO_TypeDef* col_ports[KEYPAD_COLS];
    uint16_t col_pins[KEYPAD_COLS];
} keypad_handle_t;

/**
 * @brief Inicializa el keypad configurando las filas como salidas en bajo.
 */
void keypad_init(keypad_handle_t* keypad);
/**
 * @brief Escanea una columna específica del keypad.
 * @param keypad Puntero a la estructura del keypad.
 * @param col_pin Pin de la columna a escanear.
 * @return El carácter de la tecla presionada o '\0' si no hay tecla.
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin);

#endif // KEYPAD_DRIVER_H
