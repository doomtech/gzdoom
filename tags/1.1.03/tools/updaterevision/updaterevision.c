/* updaterevision.c
 *
 * Public domain. This program uses the svnversion command to get the
 * repository revision for a particular directory and writes it into
 * a header file so that it can be used as a project's build number.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int main(int argc, char **argv)
{
	char *name;
	char currev[64], lastrev[64], run[256], *rev;
	unsigned long urev;
	FILE *stream = NULL;
	int gotrev = 0, needupdate = 1;

	if (argc != 3)
	{
		fprintf (stderr, "Usage: %s <repository directory> <path to svnrevision.h>\n", argv[0]);
		return 1;
	}

	// Use svnversion to get the revision number. If that fails, pretend it's
	// revision 0. Note that this requires you have the command-line svn tools installed.
	sprintf (run, "svnversion -cn %s", argv[1]);
	if ((name = tempnam(NULL, "svnout")) != NULL &&
		(stream = freopen(name, "w+b", stdout)) != NULL &&
		system(run) == 0 &&
		errno == 0 &&
		fseek(stream, 0, SEEK_SET) == 0 &&
		fgets(currev, sizeof currev, stream) == currev &&
		(isdigit(currev[0]) || (currev[0] == '-' && currev[1] == '1')))
	{
		gotrev = 1;
	}
	if (stream != NULL)
	{
		fclose (stream);
		remove (name);
	}
	if (name != NULL)
	{
		free (name);
	}

	if (!gotrev)
	{
		fprintf (stderr, "Failed to get current revision: %s\n", strerror(errno));
		strcpy (currev, "0");
		rev = currev;
	}
	else
	{
		rev = strchr (currev, ':');
		if (rev == NULL)
		{
			rev = currev;
		}
		else
		{
			rev += 1;
		}
	}

	stream = fopen (argv[2], "r");
	if (stream != NULL)
	{
		if (!gotrev)
		{ // If we didn't get a revision but the file does exist, leave it alone.
			fclose (stream);
			return 0;
		}
		// Read the revision that's in this file already. If it's the same as
		// what we've got, then we don't need to modify it and can avoid rebuilding
		// dependant files.
		if (fgets(lastrev, sizeof lastrev, stream) == lastrev)
		{
			if (lastrev[0] != '\0')
			{ // Strip trailing \n
				lastrev[strlen(lastrev) - 1] = '\0';
			}
			if (strcmp(rev, lastrev + 3) == 0)
			{
				needupdate = 0;
			}
		}
		fclose (stream);
	}

	if (needupdate)
	{
		stream = fopen (argv[2], "w");
		if (stream == NULL)
		{
			return 1;
		}
		urev = strtoul(rev, NULL, 10);
		fprintf (stream,
"// %s\n"
"//\n"
"// This file was automatically generated by the\n"
"// updaterevision tool. Do not edit by hand.\n"
"\n"
"#define SVN_REVISION_STRING \"%s\"\n"
"#define SVN_REVISION_NUMBER %lu\n",
			rev, rev, urev);
		fclose (stream);
		fprintf (stderr, "%s updated to revision %s.\n", argv[2], rev);
	}
	else
	{
		fprintf (stderr, "%s is up to date at revision %s.\n", argv[2], rev);
	}

	return 0;
}
