/*
 * amiga_sys.c: xu4's AmigaOS odds and ends.
 *
 * amiga_message(): an error the player sees when started from the
 * Workbench too (an Intuition requester; errorFatal() and errorWarning()
 * use it, src/error.cpp).
 *
 * amiga_screens_init(): GFOpenGPU's SDL2.module can leave empty "SDL"
 * screens behind after a full-screen window; they would stay in front of
 * the Workbench after the game ends. The screens there before SDL started
 * are noted, and at exit (after SDL_Quit) every new screen with no window
 * left on it and SDL's title is closed, or sent behind the Workbench if
 * Intuition refuses (from the Abbaye and Seven Kingdoms ports).
 *
 * The AmiCradle Team. GPL 2 or later, like xu4.
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdlib.h>
#include <string.h>

extern struct IntuitionBase *IntuitionBase;

void amiga_message(const char *title, const char *text)
{
	struct EasyStruct es = { sizeof(struct EasyStruct), 0, NULL, NULL, NULL };

	es.es_Title = (UBYTE *)title;
	es.es_TextFormat = (UBYTE *)"%s";
	es.es_GadgetFormat = (UBYTE *)"OK";
	EasyRequest(NULL, &es, NULL, (APTR)text);
}

#define MAX_KNOWN 32
static struct Screen *known[MAX_KNOWN];
static int known_count = -1;

static int was_known(struct Screen *s)
{
	int i;
	for (i = 0; i < known_count; i++)
		if (known[i] == s)
			return 1;
	return 0;
}

static void amiga_screens_close_leftovers(void)
{
	struct Screen *left[MAX_KNOWN];
	int n = 0, i;
	struct Screen *s;
	ULONG lock;

	if (known_count < 0)
		return;

	lock = LockIBase(0);
	for (s = IntuitionBase->FirstScreen; s && n < MAX_KNOWN; s = s->NextScreen)
		if (!was_known(s) && !s->FirstWindow && (s->Flags & SCREENTYPE) != WBENCHSCREEN
		    && s->Title && strcmp((const char *)s->Title, "SDL") == 0)
			left[n++] = s;
	UnlockIBase(lock);

	for (i = 0; i < n; i++)
		if (!CloseScreen(left[i]))
			ScreenToBack(left[i]);
	if (n)
		WBenchToFront();
}

void amiga_screens_init(void)
{
	struct Screen *s;
	ULONG lock = LockIBase(0);
	known_count = 0;
	for (s = IntuitionBase->FirstScreen; s && known_count < MAX_KNOWN; s = s->NextScreen)
		known[known_count++] = s;
	UnlockIBase(lock);
	/* Registered before screenInit_sys()'s atexit(SDL_Quit), so it runs
	 * after it. */
	atexit(amiga_screens_close_leftovers);
}
