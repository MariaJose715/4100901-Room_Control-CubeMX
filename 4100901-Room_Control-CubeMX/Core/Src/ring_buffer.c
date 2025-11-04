#include "ring_buffer.h"

/**
 * @brief Initializes the ring buffer variables to their initial values.
 * @param rb Pointer to the ring buffer structure.
 * @param buffer Pointer to the memory buffer used for the ring buffer.
 * @param capacity The maximum number of elements the ring buffer can hold.
*/

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t capacity) 
{
    rb->buffer = buffer;
    rb->head = 0;
    rb->tail = 0;
    rb->capacity = capacity;
    rb->is_full = false; // Initialize the full flag to false
}

/**
 * @brief Writes a byte of data to the ring buffer, discards the old data.
 * @param rb Pointer to the ring buffer structure.
 * @param data The byte of data to write.
 * @return true if the write was successful, false if the buffer is full.
 */

bool ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->capacity;
    if (rb->head == rb->tail) {
        // Buffer is full, overwrite the oldest data
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
    rb->buffer[rb->head] = data; // Write the new data
    rb->head = (rb->head + 1) % rb->capacity; // Move head forward
    if (rb->head == rb->tail) {
        rb->is_full = true; // if head meets tail, buffer is full
    }
    return true;
}

/**
 * @brief Reads a byte of data from the ring buffer.
 * @param rb Pointer to the ring buffer structure.
 * @param data Pointer to the variable where the read data will be stored.
 * @return true if the read was successful, false if the buffer is empty.
 */

bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail && !rb->is_full) {
        // Buffer is empty
        return false;
    }
    *data = rb->buffer[rb->tail]; //Read the data
    rb->tail = (rb->tail + 1) % rb->capacity; // Move tail forward
    rb->is_full = false; // After reading, the buffer cannot be full
    return true;
}

/**
 * @brief Returns the number of elements currently in the ring buffer.
 * @param rb Pointer to the ring buffer structure.
 * @return The number of elements in the ring buffer.
 */
uint16_t ring_buffer_count(ring_buffer_t *rb)
{
    if (rb->is_full) {
        return rb->capacity; //if full, return capacity
    }
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail; //Normal case
    }
    else {
        return (rb->capacity - rb->tail) + rb->head; // Wrap around case
    }
    
}

/**
 * @brief Checks if the ring buffer is empty.
 * @param rb Pointer to the ring buffer structure.
 * @return true if the buffer is empty, false otherwise.
 */
bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    return (rb->head == rb->tail && !rb->is_full);
}

/**
 * @brief Checks if the ring buffer is full.
 * @param rb Pointer to the ring buffer structure.
 * @return true if the buffer is full, false otherwise.
 */
bool ring_buffer_is_full(ring_buffer_t *rb)
{
    return rb->is_full;
}

/**
 * @brief Flushes the ring buffer, clearing all data.
 * @param rb Pointer to the ring buffer structure.
 */
void ring_buffer_flush(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->is_full = false;
}