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
graph TD
    subgraph MCU[STM32 NUCLEO-L476RG]
        SPI2((SPI2))
        I2C1((I2C1))
        GPIOK[GPIO<br/>Filas/Columnas]
        TIM3((TIM3 CH1 PWM))
        USART2((USART2))
        GPIORGB[GPIO<br/>LED RGB]
    end

    SPI2 --> RC522[RC522<br/>Lector RFID]
    GPIOK --> KEYPAD[Teclado 4x4]
    I2C1 --> OLED[Pantalla OLED<br/>SSD1306]
    TIM3 --> SERVO[Servo SG90<br/>Cerradura]
    USART2 --> ESP01[ESP-01<br/>WiFi / Logs]
    GPIORGB --> LEDRGB[LED RGB<br/>Estado]


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
graph TD
    MAIN[main.c<br/>Super Loop] --> ROOM[room_control.c<br/>Lógica de acceso<br/>Máquina de estados]

    ROOM --> MOD_KEYPAD[keypad.c<br/>Lectura teclado<br/>por interrupciones]
    ROOM --> MOD_RFID[rc522.c<br/>Lector RFID]
    ROOM --> MOD_SERVO[servo_control.c<br/>Control servo]
    ROOM --> MOD_OLED[ssd1306.c<br/>Display OLED]
    ROOM --> MOD_UART[ESP / UART<br/>Logs y eventos]

    MOD_KEYPAD --> RING[ring_buffer.c<br/>Buffer circular teclas]


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
stateDiagram-v2
    [*] --> WAITING_RFID

    WAITING_RFID : Esperando tarjeta RFID
    INPUT_PIN    : Ingreso de PIN
    CHECK_PIN    : Validación de PIN
    UNLOCKED     : Puerta abierta
    ACCESS_DENIED: Acceso denegado
    SYSTEM_LOCKOUT: Sistema bloqueado

    WAITING_RFID --> INPUT_PIN : Tarjeta válida
    WAITING_RFID --> ACCESS_DENIED : Tarjeta inválida

    INPUT_PIN --> CHECK_PIN : PIN completo
    CHECK_PIN --> UNLOCKED : PIN correcto
    CHECK_PIN --> ACCESS_DENIED : PIN incorrecto

    ACCESS_DENIED --> WAITING_RFID : Timeout / volver a intentar
    ACCESS_DENIED --> SYSTEM_LOCKOUT : Intentos >= 3

    UNLOCKED --> WAITING_RFID : Tiempo puerta abierta cumplido
    SYSTEM_LOCKOUT --> WAITING_RFID : Fin del tiempo de bloqueo


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

flowchart TD
    A[Inicio] --> B[Estado WAITING_RFID<br/>Esperar tarjeta]
    B --> C{Tarjeta detectada?}
    C -- No --> B

    C -- Sí --> D{UID válido?}
    D -- No --> E[ACCESS_DENIED<br/>Mostrar 'Denegado'<br/>LED rojo]
    E --> B

    D -- Sí --> F[INPUT_PIN<br/>Mostrar 'Ingrese PIN'<br/>LED amarillo]

    F --> G{PIN completo?}
    G -- No --> F

    G -- Sí --> H[CHECK_PIN<br/>Comparar con contraseña]

    H --> I{PIN correcto?}
    I -- No --> J[ACCESS_DENIED<br/>Intentos++]
    J --> K{Intentos >= 3?}
    K -- Sí --> L[SYSTEM_LOCKOUT<br/>Bloqueo temporal<br/>LED magenta]
    L --> B
    K -- No --> B

    I -- Sí --> M[UNLOCKED<br/>Acceso concedido<br/>LED verde<br/>Servo abre]
    M --> N[Esperar tiempo puertas abierta]
    N --> B

5. Estados y colores del LED RGB
flowchart LR
    WAIT[WAITING_RFID] -->|Azul| RGB1[LED RGB]
    PIN[INPUT_PIN] -->|Amarillo| RGB2[LED RGB]
    OK[UNLOCKED] -->|Verde| RGB3[LED RGB]
    DENY[ACCESS_DENIED] -->|Rojo| RGB4[LED RGB]
    LOCK[SYSTEM_LOCKOUT] -->|Magenta| RGB5[LED RGB]

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