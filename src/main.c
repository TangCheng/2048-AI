/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2015 TangCheng <tangcheng2005@gmail.com>
 *
 * 2048 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * 2048 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "models/game.h"
#include "log.h"

#define BUF_LEN	100
#define MILLIONSECONDS_PER_SECOND	1000ULL
#define MILLIONSECONDS_PER_MINUTE (MILLIONSECONDS_PER_SECOND * 60ULL)
#define MILLIONSECONDS_PER_HOUR		(MILLIONSECONDS_PER_MINUTE * 60ULL)
#define MILLIONSECONDS_PER_DAY 		(MILLIONSECONDS_PER_HOUR * 24ULL)

static inline void show_time(struct timeval *now)
{
	char buf[BUF_LEN] = {0};
	time_t t;
	struct tm *today;
	t = now->tv_sec;
	today = localtime(&t);
	strftime(buf, BUF_LEN, "%Y-%m-%d %H:%M:%S", today);
	LOG("%s.%u", buf, (uint32)(now->tv_usec / 1000));
}

static inline void show_elapsed(struct timeval *begin, struct timeval *end)
{
	uint64 elapsed = 0;
	elapsed = (end->tv_sec * 1000 + end->tv_usec / 1000);
	elapsed -= (begin->tv_sec * 1000 + begin->tv_usec / 1000);

	uint32 days = 0, hours = 0, mins = 0, secs = 0;
	days = elapsed / MILLIONSECONDS_PER_DAY;
	elapsed %= MILLIONSECONDS_PER_DAY;
	hours = elapsed / MILLIONSECONDS_PER_HOUR;
	elapsed %= MILLIONSECONDS_PER_HOUR;
	mins = elapsed / MILLIONSECONDS_PER_MINUTE;
	elapsed %= MILLIONSECONDS_PER_MINUTE;
	secs = elapsed / MILLIONSECONDS_PER_SECOND;
	elapsed %= MILLIONSECONDS_PER_SECOND;
	LOG("The program elapsed %u days, %u hours, %u mins, %u secs, %llu ms",
		days, hours, mins, secs, elapsed);
}

int main()
{
	game *g = NULL;
	struct timeval begin, end;

	if (game_create(&g) == true)
	{
		gettimeofday(&begin, NULL);
		show_time(&begin);
		game_start(g);
		gettimeofday(&end, NULL);
		show_time(&end);
		show_elapsed(&begin, &end);
	}
	game_destory(&g);

	return (0);
}
