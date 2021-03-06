#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "arg.h"

enum states {
	CONNECTED         = ( 1u << 0 ),
	DISCONNECTED      = ( 1u << 1 ),
	UNKNOWNCONNECTION = ( 1u << 2 ),
	PRIMARY           = ( 1u << 3 ),
	SECONDARY         = ( 1u << 4 ),
};

void printname(XRROutputInfo *o);
void usage(int status);
void die(const char *errstr, ...);

char *argv0;

int connection_mask;
int rank_mask;
bool match_both_states;

int
main(int argc, char *argv[])
{
	ARGBEGIN {
		case 'a':
			match_both_states = true;
			break;
		case 'c':
			connection_mask |= CONNECTED;
			break;
		case 'd':
			connection_mask |= DISCONNECTED;
			break;
		case 'p':
			rank_mask |= PRIMARY;
			break;
		case 's':
			rank_mask |= SECONDARY;
			break;
		case 'u':
			connection_mask |= UNKNOWNCONNECTION;
			break;
		case 'v':
			fprintf(stderr, VERSION "\n");
			return EXIT_SUCCESS;
		case 'h':
			usage(EXIT_SUCCESS);
		default:
			usage(EXIT_FAILURE);
	} ARGEND;

	if (argc > 0)
		usage(EXIT_FAILURE);

	if (!connection_mask != !rank_mask)
		match_both_states = true;

	if (!connection_mask)
		connection_mask = CONNECTED | DISCONNECTED | UNKNOWNCONNECTION;

	if (!rank_mask)
		rank_mask = PRIMARY | SECONDARY;

	Display *disp;
	XRRScreenResources *scrn;
	Window root;
	RROutput rrprim;
	XRROutputInfo *outp;
	XRROutputInfo *xrrprim = NULL;

	disp = XOpenDisplay(NULL);
	if (!disp)
		die("Can't open display\n");
	root = DefaultRootWindow(disp);
	scrn = XRRGetScreenResources(disp, root);
	rrprim = XRRGetOutputPrimary(disp, root);


	for (int i = 0; i < scrn->noutput; ++i)
	{
		int matching_states = 0;

		if ((rank_mask & PRIMARY   && scrn->outputs[i] == rrprim)
		||  (rank_mask & SECONDARY && scrn->outputs[i] != rrprim))
		{
			++matching_states;
		}

		if (match_both_states && matching_states < 1)
			continue;

		outp = XRRGetOutputInfo(disp, scrn, scrn->outputs[i]);

		if (connection_mask & (1u << outp->connection))
			++matching_states;

		if (matching_states > 1 || (!match_both_states && matching_states > 0))
		{
			if (scrn->outputs[i] == rrprim)
			{
				xrrprim = outp;
				continue;
			}
			printname(outp);
		}

		XRRFreeOutputInfo(outp);
	}

	if (xrrprim)
	{
		printname(xrrprim);
		XRRFreeOutputInfo(xrrprim);
	}

	XRRFreeScreenResources(scrn);
	XCloseDisplay(disp);

	return EXIT_SUCCESS;
}

void
printname(XRROutputInfo *o)
{
	printf("%s\n", o->name);
}


void
usage(int status)
{
	fprintf(stderr, "usage: %s [-acdpsuhv]\n", basename(argv0));
	exit(status);
}


void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}
