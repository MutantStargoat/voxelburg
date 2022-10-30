/*
Score-keeping for GBA with SRAM cartridge (originally part of gbatris)
Copyright (C) 2019-2022  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef SCOREDB_H_
#define SCOREDB_H_

#define NAME_SIZE	4

struct score_entry {
	char name[8];
	uint32_t time;
	uint16_t score;
	uint8_t level, unused;
} __attribute__((packed));

/* last entry is used just to keep the last entered name
 * and present it as a default choice in the highscore UI
 */
struct score_entry scores[11];

int last_score_rank;

int load_scores(void);
void save_scores(void);
void save_score(char *name, int score, int time, int level);
int is_highscore(int score);

#endif	/* SCOREDB_H_ */
