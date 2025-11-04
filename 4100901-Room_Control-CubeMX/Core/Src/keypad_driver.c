#include "keypad_driver.h"
#include "main.h" // Se incluye para poder usar las funciones HAL y manejar los pines GPIO

// Mapa de teclas del teclado matricial, con su respectiva distribución
static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'A', '3', '2', '1'},
  {'B', '6', '5', '4'},
  {'C', '9', '8', '7'},
  {'D', '#', '0', '*'}
};


/**
 * @brief Esta función configura las filas del teclado como salidas y las deja en nivel bajo.
 * @param keypad: estructura que tiene la información de pines del teclado.
 */
void keypad_init(keypad_handle_t* keypad) {
    // Configura cada fila como salida y las pone en 0 voltios (bajo)
    // Esto es necesario para que las columnas puedan detectar una pulsación
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
}

/**
 * @brief Función que detecta qué tecla fue presionada en función de la columna que activó la interrupción.
 * @param keypad: puntero a la estructura del teclado.
 * @param col_pin: número de pin que causó la interrupción.
 * @return Devuelve el carácter de la tecla presionada o '\0' si no se detectó ninguna.
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    char key_pressed = '\0';
    int col_index = -1;

    // Se elimina el antirrebote dentro de esta función para mayor rapidez.

    // Se determina qué columna generó la interrupción comparando los pines
    for (int i = 0; i < KEYPAD_COLS; i++) {
        if (col_pin == keypad->col_pins[i]) {
            col_index = i;
            break;
        }
    }

    // Si no corresponde a una columna conocida, se retorna sin hacer nada
    if (col_index == -1) {
        return '\0';
    }

    // Se espera un poco para que el estado eléctrico de las señales se estabilice
    HAL_Delay(2);

    // Escaneo de filas: se busca en cuál fila está la conexión activa
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // Todas las filas se ponen en estado alto
        for (int i = 0; i < KEYPAD_ROWS; i++) {
            HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
        }

        // Se baja solo la fila que queremos comprobar
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

        // Si la columna correspondiente sigue en bajo, encontramos la tecla
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[row][col_index];
            break;
        }
    }


    // Finalmente se vuelve a poner todo en estado bajo para esperar la próxima interrupción
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }

    return key_pressed;
}