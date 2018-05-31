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

/*
 * Fifo based on:
 * stratifylabs.co/embedded%20design%20tips/2013/10/02/Tips-A-FIFO-Buffer-Implementation/
 */

#include "fifo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libevdev/libevdev.h>

void *fifo_create(struct fifo_header *f, size_t size)
{
	f->buf = malloc(size * sizeof(struct input_event));
	f->head = 0;
	f->tail = 0;
	f->size = size;

	return f->buf;
}

void *fifo_realloc(struct fifo_header *f, size_t new_size)
{
	struct input_event *new_buf = malloc(new_size *
				sizeof(struct input_event));

	if (new_buf == NULL) {
		return new_buf;
	}

	/* Copy f->buf[0...f->head] to temp_buf. */
	memcpy(&new_buf[0], &f->buf[0], f->head * sizeof(struct input_event));

	if (f->tail != 0) {
		/* Number of elements to copy. */
		size_t num = f->size - f->tail;

		/* Copy f->buf[f->tail...f->size] to the end of temp_buf. */
		memcpy(&new_buf[new_size - num], &f->buf[f->tail],
					sizeof(struct input_event) * num);

		/* Set new tail. */
		f->tail = new_size - num;
	}

	free(f->buf);

	f->buf = new_buf;
	f->size = new_size;

	return f->buf;
}

void handle_full_fifo(struct fifo_header *fifo, size_t *resize_factor)
{
	if (*resize_factor < 2) {
		printf("Fifo buffer full, data discarded.\n");
	} else {
		printf("Fifo buffer full, reallocating.\n");

		size_t new_size = fifo->size * *resize_factor;

		if (fifo_realloc(fifo, new_size) == NULL) {
			perror("Failed to reallocate buffer");

			printf("Setting resize factor to 1.\n");
			*resize_factor = 1;
		} else {
			printf("Buffer reallocated, new size is %zuB.\n",
					new_size * sizeof(struct input_event));
		}
	}
}

int fifo_pop(struct fifo_header *f, struct input_event *output)
{
	if (f->tail != f->head) {
		*output = f->buf[f->tail];
		f->tail++;
		if (f->tail == f->size)
			f->tail = 0;
	} else {
		return -1;
	}

	return 0;
}

int fifo_push(struct fifo_header *f, const struct input_event *input)
{
	if (f->head + 1 == f->tail ||
			(f->head + 1 == f->size && f->tail == 0)) {
		return -1;
	} else {
		f->buf[f->head] = *input;
		f->head++;
		if (f->head == f->size) {
			f->head = 0;
		}
	}

	return 0;
}
