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
#include <sys/time.h>
#include <argp.h>

struct arguments {
	struct timeval delay;
	char *event_file_name;
	size_t buf_size;
	size_t resize_factor;
	int priority;
	unsigned short polling_rate;
};

error_t parse_args(int argc, char **argv, struct arguments *arg);
