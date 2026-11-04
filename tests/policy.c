/* Exercise the real compositor policy functions without a display backend. */
/* Renaming main removes C's implicit return-0 rule. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main dwl_main
#include "../dwl.c"
#undef main
#pragma GCC diagnostic pop
#include <assert.h>

static void
checkkeys(void)
{
	size_t i, j;
	for (i = 0; i < LENGTH(keys); i++)
		for (j = i + 1; j < LENGTH(keys); j++)
			assert(keys[i].keysym != keys[j].keysym ||
					CLEANMASK(keys[i].mod) != CLEANMASK(keys[j].mod));
	assert(MODKEY == WLR_MODIFIER_LOGO);
}

static void
checkmodes(void)
{
	struct wlr_output output = {0};
	struct wlr_output_mode modes[] = {
		{.width = 1920, .height = 1080, .refresh = 60000, .preferred = true},
		{.width = 3840, .height = 2160, .refresh = 60000},
		{.width = 3840, .height = 2160, .refresh = 239970},
		{.width = 3840, .height = 2160, .refresh = 144000},
		{.width = 1920, .height = 1080, .refresh = 360000},
	};
	size_t i;
	wl_list_init(&output.modes);
	assert(bestmode(&output, 3840, 2160) == NULL);
	for (i = 0; i < LENGTH(modes); i++)
		wl_list_insert(output.modes.prev, &modes[i].link);
	assert(bestmode(&output, 3840, 2160) == &modes[2]);
	assert(bestmode(&output, 2560, 1440) == &modes[0]);
	assert(bestmode(&output, 0, 0) == &modes[0]);
}

static void
checklayouts(void)
{
	struct wlr_output output = {0}; /* disabled: arrange has no rendering side effects */
	Monitor mon = {.wlr_output = &output, .lt = {&layouts[0], &layouts[1]}};
	int i;
	selmon = &mon;
	for (i = 1; i <= 6; i++) {
		cyclelayout(&(Arg){.i = 1});
		assert(mon.lt[mon.sellt] == &layouts[i % LENGTH(layouts)]);
	}
	selmon = NULL;
	cyclelayout(&(Arg){.i = 1});
}

static void
checkstack(void)
{
	struct wlr_output output = {0};
	Monitor mon = {.wlr_output = &output, .tagset = {1, 1}, .lt = {&layouts[0]}};
	Monitor other = {0};
	Client a = {.mon = &mon, .tags = 1}, b = a, c = a;
	Client hidden = {.mon = &mon, .tags = 2};
	Client floating = {.mon = &mon, .tags = 1, .isfloating = 1};
	Client remote = {.mon = &other, .tags = 1};
	Client *p, *order[] = {&a, &hidden, &floating, &remote, &b, &c};
	size_t i;
	selmon = &mon;
	for (i = 0; i < LENGTH(order); i++)
		wl_list_insert(clients.prev, &order[i]->link);
	wl_list_insert(&fstack, &a.flink);
	movestack(&(Arg){.i = 1});
	assert(b.link.next == &a.link && a.link.next == &c.link);
	assert(focustop(&mon) == &a);
	movestack(&(Arg){.i = -1});
	assert(a.link.next == &b.link);
	movestack(&(Arg){.i = -1}); /* first -> last */
	assert(c.link.next == &a.link && a.link.next == &clients);
	movestack(&(Arg){.i = 1}); /* last -> first eligible */
	assert(a.link.next == &b.link);
	p = wl_container_of(a.link.prev, p, link);
	a.isfloating = 1;
	movestack(&(Arg){.i = 1});
	assert(a.link.prev == &p->link);
	a.isfloating = 0;
	a.isfullscreen = 1;
	movestack(&(Arg){.i = 1});
	assert(a.link.prev == &p->link);
	a.isfullscreen = 0;
	mon.lt[0] = &layouts[1]; /* floating layout: no reorder */
	movestack(&(Arg){.i = 1});
	assert(a.link.prev == &p->link);
	mon.lt[0] = &layouts[0];
	b.tags = c.tags = 2; /* only one eligible client */
	movestack(&(Arg){.i = 1});
	assert(a.link.prev == &p->link);
	wl_list_init(&clients);
	wl_list_init(&fstack);
	movestack(&(Arg){.i = 1}); /* empty */
	selmon = NULL;
	movestack(&(Arg){.i = -1});
}

int
main(void)
{
	wl_list_init(&clients);
	wl_list_init(&fstack);
	wl_list_init(&mons);
	checkkeys();
	checkmodes();
	checklayouts();
	checkstack();
	puts("PASS: unique shortcuts, output modes, layout cycle, stack movement and guards");
	return 0;
}
