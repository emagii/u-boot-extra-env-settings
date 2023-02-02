/*
 * genenv.c
 * 
 * Copyright (C) 2023, Ulf Samuelsson <ulf@emagii.com>
 * 
 * MIT License
 * 
 * 
 * 
 * 
 */


#include <stdio.h>
#include <string.h>

#define	MAX_BUF	  1024*1024
#define MAX_NAMES 4096
#define LF			'\n'
#define	EOS			'\0'

#ifdef	DEBUG
#define	BEGIN		printf("%s: START\n", __FUNCTION__)
#define	END			printf("%s: END\n", __FUNCTION__)
#define	MARKER		printf("=======\n")
#else
#define	BEGIN
#define	END
#define	MARKER
#endif


typedef	char *ptr;
char	buffer[MAX_BUF];
int		run=1;

ptr		names [MAX_NAMES];
ptr		values[MAX_NAMES];

void emit(char *s)
{
	int c;
	do {
		c = *s++;
		if (c == '\n') {
			printf("<LF>\n");
		} else if (c == '\r') {
			printf("<CR>\n");
		} else {
			putchar(c);
		}
	} while (c != '\0');
	putchar(LF);
}


int get_file()
{
	int c, size;
	BEGIN;
	for (size = 0 ; size < MAX_BUF ; size++) {
		c = getchar();
		if (c == EOF) {
			buffer[size] = '\n';
			buffer[size+1] = '\0';
			END;
			return size;
		}
		buffer[size] = c;
	}
	END;
	return size;
}

int split_file(int size)
{
	int i;
	int ix = 0;
	int c0,c1;
	BEGIN;
	i  = 0;
	ix = 0;
#if	0
	if (size > 200) {
		printf("Size too large: %d\n", size);
		return 1;
	}
#endif
	if (buffer[i] == EOS) {
		return 0;
	} else {
		names[ix++] = &buffer[i];
	}
//	printf("size = %d\n", size);
	while(i < size) {
		if (buffer[i] == LF) {
			int pos = i;
			while(buffer[i] == LF) {
				i++;
			}
			if ((i - pos) > 1) { /* BREAK, since more than one LF */
				buffer[pos] = EOS;
				if (buffer[i] != EOS) {
					names[ix++] = &buffer[i];
				}
			} else if (buffer[i] == EOS) {	/* End of buffer */
				buffer[pos] = EOS;			/* remove final LF if present */
			}
		} else {
			i++;
		}
	}
	END;
	return ix;
}

void debug_names(int count)
{
#if	defined(DEBUG)
	int i;
	for (i = 0 ; i < count ; i++) {
		if (names[i] == NULL) {
			printf("%d: NULL\n", i);
		} else {
			printf("%d: ", i);
			emit(names[i]);
		}
	}
#endif
}

void indent(void)
{
	putchar('\t');
}

void hyphen(void)
{
	putchar('"');
}

void endmarker(void)
{
	printf("\\0\"");
}

void extend(void)
{
	printf(" \\\n");
}

void process_var(char *s)
{
	int c;
	indent();
	hyphen();
	do {
		c = *s++;
		if (c == EOS) {
			endmarker();
			extend();
		} else if (c == LF) {
			hyphen();
			extend();
			indent();
			hyphen();
		} else {
			putchar(c);
		}
	} while(c);
}

void print_env(int count)
{
	int i;
	printf("#define CONFIG_EXTRA_ENV_SETTINGS	\\\n");
	for(i = 0 ; i < count ; i++) {
		process_var(names[i]);
	}
	printf("\n");
}

int main(void)
{
	int i, size, count;
	size = get_file();
	buffer[size] = EOS;
	MARKER;
	count = split_file(size);
	MARKER;
//	printf("count = %d\n", count);
	debug_names(count);
	print_env(count);
	return 0;
}
