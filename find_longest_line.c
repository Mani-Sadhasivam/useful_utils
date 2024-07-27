/*
 * Program to find the longest line after preprocessing stage. Use this program
 * to find the macro DAEMONS (like min_t) that lives inside your driver and see
 * if you could do the exorcism.
 *
 * Author: Linus Torvalds <torvalds@linux-foundation.org>
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static void die(const char *reason)
{
	fprintf(stderr, "Fatal: %s\n", reason);
	exit(1);
}

static unsigned long find_line(const char *line, unsigned long len)
{
	unsigned long res = 0;
	do {
		res++;
		if (*line == '\n')
			break;
		line++;
	} while (--len);
	return res;
}

/*
 * By this time, we know the line length, and it has been
 * NUL-terminated. We can change it and use the underlying
 * data if we find a preprocessor linemarker.
 */
void track_cpp_line_number(char *line, unsigned long len,
	int *lineno, const char **filename)
{
	unsigned long num;
	char *end;

	if (line[0] != '#')
		return;
	if (line[1] != ' ')
		return;

	printf("%s\n", line);
	line += 2;
	len -= 2;

	num = strtoul(line, &end, 10);
	if (!num)
		return;
	if (end == line)
		return;

	len -= end - line;
	line = end;

	// Remove flag at the end
	if (len > 2) {
		if (line[len-2] == ' ' && isdigit(line[len-1])) {
			line[len-2] = 0;
			len -= 2;
		}			
	}

	// Remove spaces from end
	while (len > 1 && line[len-1] == ' ')
		line[--len] = 0;

	// We're done with 'len' now, don't bother */
	while (*line == ' ')
		line++;

	if (*line == '"') {
		end = strchr(++line, '"');
		if (end)
			*end = 0;
	}
	while (!strncmp(line, "./", 2))
		line += 2;

	*lineno = num-1;
	if (*line)
		*filename = line;
}

int main(int argc, char **argv)
{
	struct stat st;
	char *buf;
	unsigned long size;
	long longest_len = 0;
	char *longest_line = "";
	int longest_lineno = 0;
	const char *longest_file = "";
	const char *file;
	const char *unit;
	int lineno = 0, fd;

	fd = 0;
	file = "stdin";
	if (argc > 1) {
		file = argv[1];
		fd = open(file, O_RDONLY);
		if (fd < 0) die("open");
	}

	if (fstat(fd, &st)) die("stat");
	if (!S_ISREG(st.st_mode)) die("Not a regularfile");
	size = st.st_size;
	buf = malloc(size+1);
	if (!buf) die("malloc failed");
	if (read(fd, buf, size) != size) die("read failed");

	for (lineno = 1; size; lineno++) {
		unsigned long len;
		char *line = buf;

		/* Find the line, NUL-terminate it */
		len = find_line(buf, size);
		buf[len-1] = 0;

		buf += len;
		size -= len;

		/* Track preprocessor info */
		track_cpp_line_number(line, len-1, &lineno, &file);

		if (len <= longest_len)
			continue;

		longest_len = len;
		longest_line = line;
		longest_file = file;
		longest_lineno = lineno;
	}

	/* Limit printout of longest line arbitrarily */
	if (longest_len > 104)
		memcpy(longest_line + 100, "...", 4);

	unit = " bytes";
	if (longest_len > 10*1024) {
		unit = "kB";
		longest_len = (longest_len+512) / 1024;
	}

	printf("Longest line is %s:%d (%lu%s)\n",
		longest_file,
		longest_lineno,
		longest_len, unit);
	printf("   '%s'\n", longest_line);
	return 0;
}
