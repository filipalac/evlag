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

#include "threads.h"

#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libevdev/libevdev-uinput.h>

static pthread_mutex_t fifo_mutex = PTHREAD_MUTEX_INITIALIZER;

void *get_event(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	int rc = LIBEVDEV_READ_STATUS_SUCCESS;

	do {
		struct input_event ev;

		rc = libevdev_next_event(data->event_dev,
					 LIBEVDEV_READ_FLAG_NORMAL |
					 LIBEVDEV_READ_FLAG_BLOCKING, &ev);

		/* Handle dropped SYN. */
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			printf("Warning, syn dropped: (%d) %s\n", -rc, strerror(-rc));

			while (rc == LIBEVDEV_READ_STATUS_SYNC) {
				rc = libevdev_next_event(data->event_dev,
						LIBEVDEV_READ_FLAG_SYNC, &ev);
			}
		}

		/* Add delay to input. */
		timeradd(&ev.time, &data->args->delay, &ev.time);

		int rc_mut = pthread_mutex_lock(&fifo_mutex);

		if (rc_mut != 0) {
			printf("Failed to lock mutex: (%d) %s\n",
						rc, strerror(rc_mut));
		}

		if (fifo_push(data->fifo, &ev) < 0) {
			handle_full_fifo(data->fifo,
					   &data->args->resize_factor);

			fifo_push(data->fifo, &ev);
		}

		rc_mut = pthread_mutex_unlock(&fifo_mutex);

		if (rc_mut != 0) {
			printf("Failed to unlock mutex: (%d) %s\n",
						rc, strerror(rc_mut));
		}

	} while (rc == LIBEVDEV_READ_STATUS_SYNC ||
			rc == LIBEVDEV_READ_STATUS_SUCCESS);


	if (rc == -ENODEV) {
		printf("Device disconnected: (%d) %s\n", -rc, strerror(-rc));
	} else {
		printf("Failed to read input device: (%d) %s\n",
						-rc, strerror(-rc));
	}

	exit(EXIT_FAILURE);
}

void *write_event(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	int rc;

	while (rc = read(data->fd_rtc, NULL, sizeof(unsigned long))) {
		struct input_event ev;

		struct timeval current_time;
		gettimeofday(&current_time, NULL);

		struct timeval temp = ev.time;

		if (timercmp(&current_time, &ev.time, >=)) {
			int rc_fifo = 0;

			do {
				rc = libevdev_uinput_write_event(
						data->uinput_dev, ev.type,
						ev.code, ev.value);

				if (rc != 0) {
					printf("Failed to write uinput event: %s\n",
								strerror(-rc));
				}

				rc = pthread_mutex_lock(&fifo_mutex);

				if (rc != 0) {
					printf("Failed to lock mutex: %s\n",
								strerror(rc));
				}

				rc_fifo = fifo_pop(data->fifo, &ev);

				rc = pthread_mutex_unlock(&fifo_mutex);

				if (rc != 0) {
					printf("Failed to unlock mutex: %s\n",
								strerror(rc));
				}

			} while (timercmp(&temp, &ev.time, ==) && rc_fifo == 0);
		}
	}

	printf("Failed to read RTC device: (%d) %s\n", -rc, strerror(rc));

	exit(EXIT_FAILURE);
}
