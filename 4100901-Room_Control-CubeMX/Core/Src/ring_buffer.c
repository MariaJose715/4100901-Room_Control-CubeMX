#include "ring_buffer.h"

/**
 * @brief Inicializa el buffer circular.
 */
void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t capacity) {
    rb->buffer = buffer;
    rb->head = 0;
    rb->tail = 0;
    rb->capacity = capacity;
    rb->is_full = false;
}

/**
 * @brief Escribe un dato en el buffer si hay espacio disponible.
 */
bool ring_buffer_write(ring_buffer_t *rb, uint8_t data) {
    if (rb->is_full) return false; // No sobrescribir

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->capacity;

    if (rb->head == rb->tail) rb->is_full = true;
    return true;
}

/**
 * @brief Lee un dato del buffer (FIFO).
 */
bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail && !rb->is_full) return false; // Vacío

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->is_full = false;
    return true;
}

/**
 * @brief Devuelve el número de elementos almacenados.
 */
uint16_t ring_buffer_count(ring_buffer_t *rb) {
    if (rb->is_full) return rb->capacity;
    if (rb->head >= rb->tail) return rb->head - rb->tail;
    return rb->capacity - rb->tail + rb->head;
}

/**
 * @brief Indica si el buffer está vacío.
 */
bool ring_buffer_is_empty(ring_buffer_t *rb) {
    return (rb->head == rb->tail && !rb->is_full);
}

/**
 * @brief Indica si el buffer está lleno.
 */
bool ring_buffer_is_full(ring_buffer_t *rb) {
    return rb->is_full;
}

/**
 * @brief Limpia el buffer circular.
 */
void ring_buffer_flush(ring_buffer_t *rb) {
    rb->head = rb->tail = 0;
    rb->is_full = false;
}
