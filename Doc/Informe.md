🚪 INFORME DEL PROYECTO
Sistema de Control de Acceso RFID + PIN + Servo
Proyecto Final – 2025-2
Estructuras Computacionales – Universidad Nacional de Colombia
## 1. Integrantes

Maria José Cuadros	
Juan David Romero	
## 2. Introducción

Este proyecto implementa un sistema de control de acceso seguro basado en doble autenticación:

Autenticación por tarjeta RFID RC522

Verificación por PIN utilizando teclado matricial 4×4

Si ambos métodos son correctos, se activa un servomotor SG90 que funciona como cerradura electrónica.

El sistema incluye:

Pantalla OLED SSD1306 para interfaz gráfica

ESP-01 para registro de eventos por UART

Ring buffer para captura eficiente de teclas

Máquina de estados no bloqueante

Arquitectura modular basada en drivers

Este informe describe la arquitectura del sistema, su funcionamiento, el firmware y los diagramas del proceso.

## 3. Arquitectura de Hardware
3.1 Componentes
Componente	Función	Interfaz
STM32 NUCLEO-L476RG	Controlador principal	—
RC522 RFID	Lectura de UID	SPI2
Teclado 4×4	Captura del PIN	GPIO + EXTI
Servo SG90	Cerradura electrónica	PWM (TIM3 CH1)
OLED SSD1306	Interfaz visual	I2C1
ESP-01	Envío de eventos	UART2
LED integrado	Heartbeat del sistema	GPIO
3.2 Diagrama general del hardware
+---------------------------------------------------+
|                 STM32 NUCLEO-L476RG               |
|                                                   |
|  SPI2  <------>  RC522 (RFID)                     |
|                                                   |
|  GPIO (EXTI columnas + filas) <--> Teclado 4x4    |
|                                                   |
|  I2C1 <------> OLED SSD1306                       |
|                                                   |
|  TIM3 CH1 PWM ----> Servo SG90 (Cerradura)        |
|                                                   |
|  USART2 <------> ESP-01 (Eventos)                 |
|                                                   |
|  GPIO OUT ------------> LED Heartbeat             |
+---------------------------------------------------+

## 4. Arquitectura de Firmware
4.1 Estructura del proyecto
main.c
room_control.c / room_control.h
keypad.c / keypad.h
ring_buffer.c / ring_buffer.h
rc522.c / rc522.h
servo_control.c / servo_control.h
ssd1306.c / ssd1306_fonts
led.c / led.h


Cada módulo tiene una responsabilidad clara:

main.c → Inicialización + super-loop

room_control.c → Máquina de estados

rc522.c → Lectura de tarjeta

keypad.c → Escaneo por interrupciones

ring_buffer.c → Cola circular de teclas

servo_control.c → Movimiento suave del servo

ssd1306.c → Pantalla OLED

## 5. Máquina de Estados (basada en room_control.c)
[WAITING_RFID] ---> [INPUT_PIN] ---> [CHECK_PIN] ---> [UNLOCKED]
      ^                   |                  |                 |
      |                   | PIN parcial       | PIN incorrecto |
      |                   v                  v                 |
      |              [INPUT_PIN]      [ACCESS_DENIED]          |
      |                                   |                    |
      |                         intentos >= 3                  |
      |                                   v                    |
      +--------------------------- [SYSTEM_LOCKOUT] <-----------+

## 6. Diagrama de Flujo del Sistema
      [Inicio]
         |
         v
 [WAITING_RFID]
         |
 ¿Tarjeta detectada? -- NO --> (esperar)
         |
        SÍ
         |
 ¿UID válido?
   |         |
   NO        SÍ
   |         |
[ACCESS_DENIED]  --->  [INPUT_PIN]
         |                 |
    (3s timeout)     ¿PIN completo?
         |                 |
         v                 v
 [WAITING_RFID]     [CHECK_PIN]
                          |
                ¿PIN == contraseña?
                    |                |
                   NO               SÍ
                    |                |
           [ACCESS_DENIED]      [UNLOCKED]
                    |                |
                (3s)           Servo abre (5s)
                    |                |
                    v                v
             [WAITING_RFID] <--------

## 7. Explicación del Firmware
### 7.1 main.c – Inicialización y Loop Principal
Interrupción del teclado (captura inmediata de tecla)
else if (GPIO_Pin == KEYPAD_C1_Pin || GPIO_Pin == KEYPAD_C2_Pin || 
         GPIO_Pin == KEYPAD_C3_Pin || GPIO_Pin == KEYPAD_C4_Pin) 
{
    char key = keypad_scan(&keypad, GPIO_Pin);
    if (key != '\0') {
        ring_buffer_write(&keypad_rb, (uint8_t)key);
    }
}


Cada tecla presionada → entra al ring buffer (no se procesa aquí).

Procesamiento en el while(1)
if (ring_buffer_read(&keypad_rb, &key_val)) {
    room_control_process_key(&room_system, (char)key_val);
}


Aquí sí se analiza el PIN, sin bloquear la ejecución.

Actualización de la lógica principal
room_control_update(&room_system);


Esta función gobierna la máquina de estados.

## 7.2 room_control.c – Lógica del Sistema
Inicialización del sistema
strcpy(room->stored_password, "1234");
servo_set_angle(&door_servo, 0);
room_control_change_state(room, ROOM_STATE_WAITING_RFID);

Lectura y validación del UID
if (MFRC522_Compare(str, VALID_CARD_UID) == MI_OK) {
    room_control_change_state(room, ROOM_STATE_INPUT_PIN);
} else {
    room_control_change_state(room, ROOM_STATE_ACCESS_DENIED);
}

Procesamiento del PIN
room->input_buffer[room->input_index++] = key;

if (room->input_index == PASSWORD_LENGTH)
    room_control_change_state(room, ROOM_STATE_CHECK_PIN);

Validación del PIN
if (strcmp(room->input_buffer, room->stored_password) == 0)
    room_control_change_state(room, ROOM_STATE_UNLOCKED);
else {
    room->failed_attempts++;
    if (room->failed_attempts >= MAX_PIN_RETRIES)
        room_control_change_state(room, ROOM_STATE_SYSTEM_LOCKOUT);
    else
        room_control_change_state(room, ROOM_STATE_ACCESS_DENIED);
}

Acción del servo (apertura/cierre)
if (open)
    servo_move_slow(&door_servo, 90, 20);
else
    servo_move_slow(&door_servo, 0, 20);

Actualización de la pantalla OLED
case ROOM_STATE_WAITING_RFID:
    ssd1306_WriteString("BIENVENIDO", Font_11x18, White);
    ssd1306_WriteString("Pase Tarjeta", Font_11x18, White);
    break;

## 8. Módulo Ring Buffer (teclado no bloqueante)
bool ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    uint16_t next = (rb->head + 1) % rb->capacity;
    if (next == rb->tail) return false; // lleno
    rb->buffer[rb->head] = data;
    rb->head = next;
    return true;
}


Evita perder teclas cuando el usuario escribe rápido.

## 9. Resultados
Prueba	Resultado
Lectura RFID	20–40 ms
Validación PIN	inmediata
Apertura servo	movimiento suave
Lockout por fallos	funcional
OLED	mensajes claros según estado
UART a ESP-01	logs inmediatos
## 10. Conclusiones

Se implementó un sistema robusto de doble seguridad (RFID + PIN).

La máquina de estados garantiza orden y escalabilidad.

El uso de interrupciones y ring buffer hace que el teclado sea rápido y confiable.

El servo se comporta como una cerradura real con apertura suave.

El sistema es expandible hacia funciones IoT o administración remota.

## 11. Trabajo Futuro

Administración remota de tarjetas autorizadas

Registro histórico de accesos vía ESP-01

Implementación de cifrado en comunicación UART

Integración con app móvil