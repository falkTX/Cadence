/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>

#include "lilv/lilv.h"

#include "lilv_config.h"

void
print_version(void)
{
	printf("lilv_bench (lilv) " LILV_VERSION "\n");
	printf("Copyright 2011-2011 David Robillard <http://drobilla.net>\n");
	printf("License: <http://www.opensource.org/licenses/isc-license>\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

void
print_usage(void)
{
	printf("Usage: lilv_bench\n");
	printf("Load all discovered LV2 plugins.\n");
}

int
main(int argc, char** argv)
{
	LilvWorld* world = lilv_world_new();
	lilv_world_load_all(world);

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	LILV_FOREACH(plugins, p, plugins) {
		const LilvPlugin* plugin = lilv_plugins_get(plugins, p);
		lilv_plugin_get_class(plugin);
	}

	lilv_world_free(world);

	return 0;
}
