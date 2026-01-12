/**
 * @file circular_buffer.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2023-09-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __CIRCULAR_BUFFER_H_
#define __CIRCULAR_BUFFER_H_

#include "main.h"
#ifdef ECE353_FREERTOS 
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    volatile uint32_t produce_count;
    volatile uint32_t consume_count;
    uint16_t max_size;
    volatile char *data;
} circular_buffer_t ;


//*****************************************************************************
// Initializes a circular buffer.
//
//      1. Initialize the produce_count
//      2. Initialize the consume_count
//      3. Initialize the max_size
//      4. malloc enough memory from the heap for the data array
//
// Parameters
//    buffer  :   The address of the circular buffer.
//    max_size:   Number of entries in the circular buffer.
//*****************************************************************************
circular_buffer_t * circular_buffer_init(uint16_t max_size);

//*****************************************************************************
// Returns a circular buffer to the heap
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
void circular_buffer_delete(circular_buffer_t * buffer);

//*****************************************************************************
// Adds a character to the circular buffer.
//
// Parameters
//    buffer  :   The address of the circular buffer.
//    c       :   Character to add.
//*******************************************************************************
bool circular_buffer_add(circular_buffer_t *buffer, char c);

//*****************************************************************************
// Removes the oldest character from the circular buffer.
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
bool circular_buffer_remove(circular_buffer_t *buffer, char *c);

//*****************************************************************************
// Returns true if the circular buffer is empty.  Returns false if it is not.
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
bool circular_buffer_empty(circular_buffer_t *buffer);

//*****************************************************************************
// Returns true if the circular buffer is full.  Returns false if it is not.
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
bool circular_buffer_full(circular_buffer_t *buffer);

//*****************************************************************************
// Returns the number of bytes currently in the circular buffer 
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
uint32_t circular_buffer_get_num_bytes(circular_buffer_t *buffer);

//*****************************************************************************
// Resets the produce and consume counts for the circular buffer 
//
// Parameters
//    buffer  :   The address of the circular buffer.
//*****************************************************************************
void circular_buffer_reset(circular_buffer_t *buffer);

#endif // ECE353_FREERTOS
#endif /* circular_buffer_H_ */
