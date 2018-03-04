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

#include "arg_parse.h"
#include "fifo.h"
#include "threads.h"

#include <fcntl.h>
#include <linux/rtc.h>
#include <pthread.h>
#include <string.h>

int main(int argc, char **argv)
{
	/* Set default values and parse arguments. */
	struct arguments args;

	args.event_file_name = NULL;
	args.delay.tv_sec = 0;
	args.delay.tv_usec = 0;
	args.buf_size = 0;
	args.resize_factor = 2;
	args.priority = 20;
	args.polling_rate = 2048;

	if (parse_args(argc, argv, &args) < 0) {
		perror("Failed to parse arguments");
		exit(EXIT_FAILURE);
	}


	/* Set scheduler priority. */
	struct sched_param sched;
	sched.sched_priority = args.priority;

	if (sched_setscheduler(0, SCHED_FIFO, &sched) < 0) {
		perror("Failed to set scheduler, check your privilege");
	}


	/* Open device. */
	int fd_event = open(args.event_file_name, O_RDONLY);

	if (fd_event < 0) {
		perror("Failed to open input device");
		exit(EXIT_FAILURE);
	}


	/* Create libevdev device and grab it. */
	struct libevdev *event_dev = NULL;

	if (libevdev_new_from_fd(fd_event, &event_dev) < 0) {
		perror("Failed to init libevdev");
		exit(EXIT_FAILURE);
	}

	if (libevdev_grab(event_dev, LIBEVDEV_GRAB) < 0) {
		perror("Failed to grab device");
		exit(EXIT_FAILURE);
	}


	/* Create uinput clone of device. */
	int fd_uinput = open("/dev/uinput", O_WRONLY);

	if (fd_uinput < 0) {
		perror("Failed to open uinput device");
		exit(EXIT_FAILURE);
	}

	struct libevdev_uinput *uinput_dev = NULL;

	if (libevdev_uinput_create_from_device(event_dev,
				fd_uinput, &uinput_dev) < 0) {
		perror("Failed to create uinput device");
		exit(EXIT_FAILURE);
	}


	/* Create RTC interrupts. */
	int fd_rtc = open("/dev/rtc", O_RDONLY);

	if (fd_rtc < 0) {
		perror("Failed to open RTC timer");
		exit(EXIT_FAILURE);
	}

	if (ioctl(fd_rtc, RTC_IRQP_SET, args.polling_rate) < 0) {
		perror("Failed to set RTC interrupts");
		exit(EXIT_FAILURE);
	}

	if (ioctl(fd_rtc, RTC_PIE_ON, 0) < 0) {
		perror("Failed to enable  RTC interrupts");
		exit(EXIT_FAILURE);
	}


	/* Create fifo buffer. */
	struct fifo_header fifo;

	if (fifo_create(&fifo, args.buf_size) == NULL) {
		perror("Failed to allocate fifo buffer");
		exit(EXIT_FAILURE);
	}


	/* Set data for threads. */
	struct thread_data data;

	data.fifo = &fifo;
	data.args = &args;
	data.event_dev = event_dev;
	data.uinput_dev = uinput_dev;
	data.fd_rtc = fd_rtc;


	/* Create threads. */
	pthread_t thr[2];

	int rc = pthread_create(&thr[0], NULL, get_event, &data);

	if (rc != 0) {
		printf("Failed to create thread0: (%d) %s\n", rc, strerror(rc));
		exit(EXIT_FAILURE);
	}

	rc = pthread_create(&thr[1], NULL, write_event, &data);

	if (rc != 0) {
		printf("Failed to create thread1: (%d) %s\n", rc, strerror(rc));
		exit(EXIT_FAILURE);
	}

	/* Join threads. */
	rc = pthread_join(thr[0], NULL);

	if (rc != 0) {
		printf("Failed join thread0: (%d) %s\n", rc, strerror(rc));
		exit(EXIT_FAILURE);
	}

	rc = pthread_join(thr[1], NULL);

	if (rc != 0) {
		printf("Failed join thread1: (%d) %s\n", rc, strerror(rc));
		exit(EXIT_FAILURE);
	}
}
