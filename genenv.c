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
/* We can only define this number of environment variables */
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
typedef struct {
	int	pos;		/* Start in buffer for this variable */
	int last;		/* Start in buffer for next variable */
	int valid;		/* Has been defined */
	char name[128];
} var_t;

char	buffer[MAX_BUF];
int		run=1;



ptr		names [MAX_NAMES];
var_t	variables[MAX_NAMES];

int idchar(char c)
{
	if ((c >= 'a') && (c <= 'z'))
		return 1;
	if ((c >= 'A') && (c <= 'Z'))
		return 1;
	if ((c >= '0') && (c <= '9'))
		return 1;
	if (c == '_')
		return 1;
	return 0;
}

void defvar(var_t *v, int vix)
{
	var_t *this = &variables[vix];
	strcpy(this->name, v->name);
	this->pos = v->pos;
	if (this->name[0] != EOS) {
		this->valid = 1;
	} else {
		this->valid = 0;
	}
	this->last= -1;
	if (vix != 0) {
		variables[vix-1].last = v->pos-1;
	}
}

int get_file()
{
	int c, size, linefeed, skip;
	int current_var;
	var_t var;
	current_var=0;
	linefeed=1;
	skip=0;
	BEGIN;
	for (size = 0 ; size < MAX_BUF ; ) {
		c = getchar();
		if (c == EOF) {
			buffer[size] = '\n';
			buffer[size+1] = '\0';
			/* Define an empty variable at the end */
			var.name[0] = EOS;
			var.pos = size + 1;
			defvar(&var, current_var);
			END;
			return size;
		} else if (skip == 1) {
			if (c == LF) {
				linefeed=1;
				skip=0;
			}
			/* if (c == EOF) we will get it again during next getchar() */
		} else {
			if (linefeed == 1) {
				/* Check for comment of variable definition */
				if (c == '#') {
					skip=1;
					c = getchar();
					if (c == '!') {	/* Define a variable name */
						int vix = 0;
						while(1) {
							c = getchar();
							if (idchar(c)) {
								var.name[vix++] = c;
							} else {
								break;
							}
						}
						var.name[vix++] = EOS;
						var.pos = size;
						defvar(&var, current_var++);
					}
				} else {
					skip=0;
				}
			}
			if (skip != 1) {
				buffer[size++] = c;
			}
			if (c == LF) {
				linefeed = 1;
				skip=0;
			} else {
				linefeed = 0;
			}
		}
	}
	END;
	return size;
}

int split_file(int start, int stop)
{
	int i;
	int ix = 0;
	int c0,c1;
	BEGIN;
	i  = start;
	for (ix = 0; ix < MAX_NAMES ; ix++) {
		names[ix++] = NULL;
	}
	ix = 0;
	if (buffer[i] == EOS) {
		return 0;
	} else {
		names[ix++] = &buffer[i];
	}
	while(i <= stop) {
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

void print_env(char *s, int count)
{
	int i;
	printf("#define %s	\\\n", s);
	for(i = 0 ; i < count ; i++) {
		process_var(names[i]);
	}
	printf("\n");
}

void print_var(var_t *v)
{
	int i;
	printf("pos=%-5d-%5d: %s\n", v->pos, v->last, v->name);
	for (i = v->pos ; i < v->last ; i++) {
		putchar(buffer[i]);
	}
	putchar(LF);
}

void print_vars(void)
{
	int i, count;
	for (i = 0; i < MAX_NAMES ; i++) {
		var_t *this;
		this = &variables[i];
		if (this->valid) {
//			print_var(this);
			count = split_file(this->pos, this->last);
			print_env(this->name, count);
		}
	}
}
int main(void)
{
	int i, size, count;
	size = get_file();
	buffer[size] = EOS;
	print_vars();
	return 0;
}
