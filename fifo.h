/*
 * evlag 
 * Copyright 2018 Filip Aláč
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <stdlib.h>
#include <libevdev/libevdev.h>

struct fifo_header {
	struct input_event *buf;
	size_t head;
	size_t tail;
	size_t size;
};

/*
 * This function allocate ram according to size(size is
 * not in bytes, but in number of struct input_event
 * packets!). Function returns a pointer to allocated buffer.
 */
void *fifo_create(struct fifo_header *f, size_t size);

/*
 * This function reallocate buffer and returns
 * a pointer to new buffer. If reallocation failed,
 * old buffer is kept and function return NULL.
 */
void *fifo_realloc(struct fifo_header *f, size_t new_size);

/*
 * This function is handling full fifo buffer.
 * If resize_factor is set to 0(or 1), buffer
 * discard new data when full, if not, size is
 * multiplied by resize_factor. If realloc can't
 * allocate more ram, resize_factor is set to 1.
 */
void handle_full_fifo(struct fifo_header *fifo, size_t *resize_factor);

/*
 * This function write element from tail to *output.
 * If fifo is empty, return -1, else return 0.
 */
int fifo_pop(struct fifo_header *f, struct input_event *output);

/*
 * This function write element from *input to head.
 * If fifo is full, return -1, else return 0.
 */
int fifo_push(struct fifo_header *f, const struct input_event *input);
