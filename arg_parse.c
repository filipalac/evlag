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

#include <stdlib.h>
#include <sys/time.h>
#include <argp.h>
#include <libevdev/libevdev.h>

const char *argp_program_version =
	"evlag 1.0";

const char *argp_program_bug_address =
	"<filipalac@gmail.com>";

static char doc[] =
	"evlag 1.0 -- A simple tool for simulating input lag\n"
	"\nIf resize factor is set to 0 or 1, buffer won't resize.\n"
	"Don't forget to run as superuser.";

static char args_doc[] =
	"--device <FILE> --lag <NUM>";

static struct argp_option options[] = {
	{"device", 'd', "FILE", 0, "/dev/input/eventX"},
	{"lag", 'l', "NUM", 0, "Set length of delay(ms)"},
	{"buffer", 'b', "NUM", 0, "Set size of buffer(MiB)"},
	{"Hz", 'h', "NUM", 0, "Set polling rate of uinput device(1 - 8192)"},
	{"resize", 'r', "NUM", 0, "Set resize factor of full buffer"},
	{"priority", 'p', "NUM", 0, "Set scheduler priority(1 - 99)"},
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case 'd':
		args->event_file_name = arg;
		break;
	case 'l':;
		unsigned long long m_sec = strtoull(arg, NULL, 10);
		time_t sec = m_sec / 1000;

		args->delay.tv_sec = sec;
		args->delay.tv_usec = (m_sec - sec * 1000) * 1000;
		break;
	case 'b':
		args->buf_size = 1048576 * strtoull(arg, NULL, 10) /
			sizeof(struct input_event);
		break;
	case 'h':
		args->polling_rate = strtol(arg, NULL, 10);
		break;
	case 'r':
		args->resize_factor = strtoull(arg, NULL, 10);
		break;
	case 'p':
		args->priority = strtol(arg, NULL, 10);
		break;
	case ARGP_KEY_END:
		/* Check if file is specified. */
		if (args->event_file_name == NULL) {
			argp_state_help(state, stdout, ARGP_HELP_STD_HELP);

		}

		/* Check if delay is specified. */
		if (args->delay.tv_sec == 0 && args->delay.tv_usec == 0) {
			argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
		}

		/*
		 * If buffer size is not specified, calculate buffer size by:
	 	 * [assumed number of packets per ms] * [time of delay in ms]
		 */
		if (args->buf_size == 0) {
			args->buf_size += 10 * args->delay.tv_sec * 1000;
			args->buf_size += 10 * args->delay.tv_usec / 1000;
		}
		break;
	}

	return 0;
}

error_t parse_args(int argc, char **argv, struct arguments *args)
{
	struct argp argp = {options, parse_opt, args_doc, doc};

	return argp_parse(&argp, argc, argv, 0, 0, args);
}
