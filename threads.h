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

#include "arg_parse.h"
#include "fifo.h"

#include <libevdev/libevdev-uinput.h>

/*
 * Structure which is passed to get_event
 * and write_event functions.
 */
struct thread_data {
	struct fifo_header *fifo;
	struct arguments *args;
	struct libevdev *event_dev;
	struct libevdev_uinput *uinput_dev;
	int fd_rtc;
};

/*
 * This function is receiving events, adding delay
 * to them and writing them to fifo buffer.
 */
void *get_event(void *arg);

/*
 * This function is receiving events from fifo buffer,
 * comparing fifo event timeval with current time, and
 * when the current time is same or bigger than the event
 * time, write the events to uinput device.
 */
void *write_event(void *arg);
