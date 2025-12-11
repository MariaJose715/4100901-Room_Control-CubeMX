🚪 INFORME DEL PROYECTO FINAL – Sistema de Control de Acceso con Doble Seguridad (RFID + PIN + Servo)
Curso: Estructuras Computacionales (4100901)
Universidad Nacional de Colombia – Sede Manizales
## 1. Integrantes
Nombre	Rol	Correo
Maria José Cuadros	Integración HW/FW	ncuadroso@unal.edu.co

Juan David Romero	Desarrollo Firmware	—
## 2. Resumen del Proyecto

Este proyecto implementa un sistema de control de acceso seguro basado en la placa NUCLEO-L476RG, utilizando doble autenticación:

Tarjeta RFID RC522

PIN de 4 dígitos en teclado matricial 4×4

Si ambas verificaciones son correctas, un servomotor SG90 actúa como cerradura electrónica, abriendo la puerta durante un tiempo determinado.

El sistema también incluye:

Pantalla OLED SSD1306 para la interfaz visual

ESP-01 para envío de eventos por UART

LED RGB como indicador visual del estado

Arquitectura basada en super loop no bloqueante + máquina de estados

Este diseño reemplaza la función de climatización del proyecto original y la adapta a un sistema de seguridad física.

## 3. Arquitectura del Sistema
### 3.1 Arquitectura de Hardware
+---------------------------------------------------+
|                 STM32 NUCLEO-L476RG               |
|                                                   |
|  SPI2  <------>  RC522 (RFID)                     |
|                                                   |
|  GPIO (EXTI columnas + filas) <--> Teclado 4x4    |
|                                                   |
|  I2C1 <------> Pantalla OLED SSD1306              |
|                                                   |
|  TIM3 CH1 PWM ----> Servo SG90 (Cerradura)        |
|                                                   |
|  USART2 <------> ESP-01 (Eventos/Logs)            |
|                                                   |
|  GPIO OUT ------------> LED RGB                   |
+---------------------------------------------------+

3.1.1 Descripción de Componentes
Componente	Función	Interfaz
RC522 RFID	lectura de UID	SPI2
Teclado matricial 4×4	ingreso del PIN	GPIO + EXTI
OLED SSD1306	interfaz gráfica	I2C1
Servo SG90	cerradura electrónica	PWM (TIM3)
ESP-01	transmisión de eventos	UART2
LED RGB	indicador del estado	GPIO
### 3.2 Arquitectura de Firmware
3.2.1 Diagrama de Bloques del Firmware
+---------------------------------------------------------------+
|                       room_control.c                          |
|---------------------------------------------------------------|
| - Máquina de estados                                           |
| - Validación RFID + PIN                                        |
| - Control del servo                                             |
| - Actualización OLED                                            |
| - Envío de eventos al ESP-01                                   |
+---------------------------------------------------------------+

                |                 |                |
                v                 v                v

+------------------------+   +------------------------+   +------------------------+
|      keypad.c          |   |      rc522.c           |   |  servo_control.c       |
|------------------------|   |------------------------|   |------------------------|
| - Lectura por EXTI     |   | - Lectura UID RFID     |   | - PWM para servo       |
| - Scan filas/columnas  |   | - Anticolisión         |   | - Movimiento suave     |
| - Envío a ring buffer  |   | - SPI2                 |   | - Ángulos              |
+------------------------+   +------------------------+   +------------------------+

                |
                v

+------------------------+
|     ring_buffer.c      |
|------------------------|
| - FIFO no bloqueante   |
| - Almacena teclas      |
+------------------------+

                |
                v

+------------------------+
|      ssd1306.c         |
|------------------------|
| - Mensajes por estado  |
| - Textos y gráficos    |
+------------------------+

                |
                v

+------------------------+
|  Comunicación UART     |
|------------------------|
| - Logs a ESP-01        |
| - Reporte de eventos   |
+------------------------+

3.2.2 Patrón de Diseño: Super Loop No Bloqueante

Tu main.c sigue exactamente el patrón recomendado:

while (1) {
    heartbeat();                           // LED de vida
    room_control_update(&room_system);     // lógica principal
    if (ring_buffer_read(&keypad_rb,&key)) {
        room_control_process_key(&room_system, key);
    }
    HAL_Delay(10);
}


✔ No bloquea la ejecución
✔ Llama funciones por evento
✔ Es ideal para sistemas embebidos reactivos

3.2.3 Máquina de Estados (Definida en room_control.c)
[WAITING_RFID] ---> [INPUT_PIN] ---> [CHECK_PIN] ---> [UNLOCKED]
      ^                   |                  |                 |
      |                   | PIN parcial       | PIN incorrecto |
      |                   v                  v                 |
      |              [INPUT_PIN]      [ACCESS_DENIED]          |
      |                                   |                    |
      |                         intentos >= 3                  |
      |                                   v                    |
      +--------------------------- [SYSTEM_LOCKOUT] <-----------+

## 4. Descripción Funcional del Sistema
4.1 Control de Acceso RFID

En process_rfid_check():

if (MFRC522_Compare(str, VALID_CARD_UID) == MI_OK) {
    room_control_change_state(room, ROOM_STATE_INPUT_PIN);
} else {
    room_control_change_state(room, ROOM_STATE_ACCESS_DENIED);
}


✔ Detecta tarjetas
✔ Lee UID
✔ Compara con la tarjeta autorizada
✔ Cambia el estado según el resultado

4.2 Control de Acceso por PIN

En room_control_process_key():

room->input_buffer[room->input_index++] = key;

if (room->input_index == PASSWORD_LENGTH)
    room_control_change_state(room, ROOM_STATE_CHECK_PIN);


✔ Lee teclas desde el ring buffer
✔ Enmascara entrada en OLED
✔ Cuando se ingresa el PIN completo → se valida

4.3 Validación del PIN
if (strcmp(room->input_buffer, room->stored_password) == 0)
    room_control_change_state(room, ROOM_STATE_UNLOCKED);
else {
    room->failed_attempts++;
    ...
}


✔ PIN incorrecto → estado ACCESS_DENIED
✔ 3 errores → estado SYSTEM_LOCKOUT

4.4 Control del Servo (Cerradura)
if (open)
    servo_move_slow(&door_servo, 90, 20);
else
    servo_move_slow(&door_servo, 0, 20);


✔ Movimiento suave
✔ 90° = puerta abierta
✔ 0° = cerrada
✔ Se cierra automáticamente tras 5 segundos

4.5 Interfaz OLED

Según el estado:

"BIENVENIDO / Pase Tarjeta"
"PIN: ****"
"ACCESO CONCEDIDO"
"DENEGADO"
"BLOQUEADO"

4.6 Envío de Eventos por ESP-01
sprintf(buffer, "{ evento: %s, uid: %s, clave: %s }\r\n");
HAL_UART_Transmit(&huart2, buffer, strlen(buffer), 50);


✔ Logs de acceso
✔ UID detectado
✔ PIN ingresado
✔ Estados críticos como lockout

## 5. Protocolo de Comandos (extensible)

Aunque tu versión no usa control remoto completo, el sistema soporta:

Comando	Función
UNLOCK	Fuerza apertura
GET_STATUS	Devuelve estado
SET_PASS:XXXX	Cambia PIN
PING	Verifica conexión
## 6. Optimización Aplicada

Uso de interrupciones → teclas capturadas sin perder ninguna

Ring buffer → lectura no bloqueante

Máquina de estados → simplifica flujo

Servo con movimiento lento → evita daños mecánicos

OLED actualizado solo cuando se requiere

UART por interrupciones

## 7. Resultados
Función	Resultado
Lectura RFID	Precisa y rápida
Validación PIN	Inmediata
Servo	Movimiento suave y controlado
OLED	Mensajes claros por estado
Lockout	Funciona según intentos
Eventos UART	Reportes correctos
## 8. Conclusiones

Se logró un sistema de doble seguridad funcional (RFID + PIN).

El servo simula una cerradura electrónica de manera efectiva.

La máquina de estados hace que el sistema sea estable y predecible.

La arquitectura modular facilita mantenimiento y futuras expansiones.

Se integra hardware, firmware y comunicación serial de forma profesional.

## 9. Trabajo Futuro

Soporte para múltiples tarjetas autorizadas

Registro histórico de accesos

Control remoto vía WiFi (panel web)

Doble servo o cierre electromagnético real