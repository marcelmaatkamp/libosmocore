/*
 * (C) 2010 by Holger Hans Peter Freyther
 * (C) 2010 by On-Waves
 * All Rights Reserved
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
 */

#include <osmocom/core/msgfile.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>

#include <stdio.h>

static void dump_entries(struct osmo_config_list *entries)
{
	struct osmo_config_entry *entry;

	if (!entries) {
		fprintf(stderr, "Failed to parse the file\n");
		return;
	}

	llist_for_each_entry(entry, &entries->entry, list) {
		printf("Entry '%s:%s:%s:%s'\n",
			entry->mcc, entry->mnc, entry->option, entry->text);
	}
}

int main(int argc, char **argv)
{
	struct osmo_config_list *entries;

	OSMO_ASSERT(argc > 1);
	entries = osmo_config_list_parse(NULL, argv[1]);
	dump_entries(entries);
	talloc_free(entries);

	return 0;
}
