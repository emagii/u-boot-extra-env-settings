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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#define	MAX_BUF	  1024*1024
/* We can only define this number of environment variables */
#define MAX_NAMES 4096
#define LINE_SIZE 1024
#define LF			'\n'
#define VAR			'V'
#define	EOS			'\0'
#define FF			'\014'

#ifdef	DEBUG
#define	BEGIN		printf("%s: START\n", __FUNCTION__)
#define	END			printf("%s: END\n", __FUNCTION__)
#define	MARKER		printf("=======\n")
#else
#define	BEGIN
#define	END
#define	MARKER
#endif

typedef struct {
	char		*data;
	FILE		*file;
	int32_t		fd;
	char		*name;
	uint32_t	size;
	uint32_t	open;
} filedsc_t;

filedsc_t	infile;		/* We read the instructions from here */
filedsc_t	outfile;	/* Result */
filedsc_t	editfile;	/* We insert the result here, if present */

typedef	char *ptr;
typedef struct {
	int	pos;		/* Start in buffer for this variable */
	int last;		/* Start in buffer for next variable */
	int valid;		/* Has been defined */
	char name[128];
} var_t;

char	buffer[MAX_BUF];
int		buffer_ix =  0;

var_t	variables[MAX_NAMES];
int		variable_ix = 0;		/* index to next free variable */
int		variable_pos = 0;

ptr		names [MAX_NAMES];
int		names_ix = 0;

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

void endvar(void)
{
	if (variable_ix > 0) {	/* vix = 0 is the first entry, no predecessor */
		variables[variable_ix - 1].last = buffer_ix - 1;
	}
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
//	printf("%s: valid=%d\n", this->name, this->valid);

	this->last= -1;
	if (vix != 0) {
		variables[vix-1].last = v->pos-1;
	}
}

void process_comment(void)
{
	char	line[LINE_SIZE];
	int		c;
	int		ix = 0;
	var_t	v;
	c = getchar();
	if (c == '!') {
		c = getchar();
		while (idchar(c)) {
			line[ix++] = c;
			c = getchar();
		}
		line[ix] = EOS;
		while(c >= ' ') {
			c = getchar();
		}
		printf("VARIABLE: %s\n", line);
		strcpy(v.name, line);
		v.pos = buffer_ix;
		endvar();
		defvar(&v, variable_ix++);
	} else {
		while (c >= ' ') {
			line[ix++] = c;
			c = getchar();
		}
		line[ix] = EOS;
		printf("COMMENT: %s\n", line);
	}
}

int get_line(int begin)
{
	char	line[LINE_SIZE];
	int		i, c, status;
	var_t	v;
	status = EOS;
	while ((c = getchar()) == '#') {
		process_comment();
	}
	ungetc(c, stdin);
	for (i = 0; i < LINE_SIZE ; i++) {
		c = getchar();
		if (c == EOF) {
			if (i == 0) {
				status = EOF;	/* Invalid Line */
				line[i] = EOS;
			} else {
				status = FF;	/* End of field */
				line[i] = EOS;
			}
			break;
		} else if (c == LF) {
			int lines = 1;
			while (1) {			/* Skip multiple LF, and report FF */
				c = getchar();
				if (c != LF) {
					ungetc(c, stdin);
					break;
				} else {
					lines++;
				}
			}
			if (i == 0) {		/* LF found directly */
				status = FF;	/* At least 2 x LF == End of field */
				line[i] = EOS;
			} else {
				if (lines > 1) {	/* Number of LFs */
					status = FF;
					line[i] = EOS;
				} else {
					status = LF;	/* 1 x LF Valid line */
					line[i] = EOS;
//					line[i] = LF;
//					line[i+1] = EOS;
				}
			}
			break;
		} else {
			line[i] = c;
		}
	}
//	printf(">%s\n", line);

	switch(status) {
	case LF:
		if (begin == 0) {
			buffer[buffer_ix++] = LF;
		}
		strcpy(&buffer[buffer_ix], line);
		buffer_ix += strlen(line);
		break;
	case FF:
		strcpy(&buffer[buffer_ix], line);
		buffer_ix += strlen(line);
		/* Nothing to copy */
		break;
	case VAR:
		break;
	case EOF:
		/* Nothing to copy */
		break;
	case EOS:
		printf("%s: Buffer Overrun\n", __FUNCTION__);
		exit(1);
		break;
	default:
		printf("%s: BAD VALUE %d\n", __FUNCTION__, status);
		exit(1);
		break;
	}
	return status;
}

int get_lines(void)
{
	int status;
	int i = 0;
	int begin = 1;
	do {
		status = get_line(begin);
		if (status == FF) {
			names[names_ix] = &buffer[variable_pos];
			buffer[buffer_ix++] = EOS;
			variable_pos = buffer_ix;
			begin = 1;
		} else { /* LF */
			begin = 0;
		}
		i++;
	} while (status != EOF);

	return buffer_ix;
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
	int c;
	BEGIN;
//	printf("split_file[%d:%d]\n", start, stop);
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
	while(i < stop) {
		if (buffer[i] == LF) {
			int pos = i;
			while(buffer[i] == LF) {
				i++;
			}
			if (i < stop) {
				if ((i - pos) > 1) { /* BREAK, since more than one LF */
					buffer[pos] = EOS;
					if (buffer[i] != EOS) {
						names[ix++] = &buffer[i];
					}
				} else if (buffer[i] == EOS) {	/* End of buffer */
					buffer[pos] = EOS;			/* remove final LF if present */
				}
			}
		} else {
			i++;
		}
	}
	END;
#if	0
	printf("count=%d [%d:%d]\n", ix, start, i);
	for (i = 0; i < ix ; i++) {
		printf("names[%d] = \"%s\"\n", i, names[i]);
	}
	buffer[stop] = EOS;
	c = buffer[stop];
	printf("last=%d,'%c'\n", c,c);
#endif
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



void hexprint(int start, int stop)
{
	int i;
	for (i = start & ~0x7 ; i < ((stop+8) & ~7) ; i++) {
		if ((i & 0x07) == 0) {
			printf("0x%04X: ", i);
		}
		if ((i < start) || (i > stop)) {
			printf("     ");
		} else {
			printf("0x%02X ", buffer[i]);
		}
		if ((i & 0x7) == 0x7) {
			int p0 = i & ~7;
			int p1 = p0 + 8;
			printf("\t\t\"");
			for (int j = p0 ; j < p1 ; j++) {
				if ((j >= start) && (j <= stop)) {
					if (buffer[j] >= ' ') {
						putchar(buffer[j]);
					} else {
						putchar('.');
					}
				} else {
					putchar(' ');
				}
			}
			putchar('"');
			printf("\n");
		}
	}
	if ((i & 0x7) != 0x0) {
		printf("\n");
	}
}

void print_var(var_t *v)
{
	int i;
	printf("pos=%04X-%04X: %s\n", v->pos, v->last, v->name);
	hexprint(v->pos, v->last);
#if	1
	for (i = v->pos ; i < v->last ; i++) {
		if (buffer[i] == EOS) {
			putchar(LF);
		} else  {
			putchar(buffer[i]);
		}
	}
	putchar(LF);
#endif
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
			hexprint(this->pos, this->last);
//			printf("count=%d\n", count);
			print_env(this->name, count);
		}
	}
}

void print_short_vars(void)
{
	int i, count;
	printf("==================\n");
	for (i = 0; i < MAX_NAMES ; i++) {
		var_t *this;
		this = &variables[i];
		if (this->valid) {
			printf("======\n");
			print_var(this);
		}
	}
}

uint32_t test ()
{
	int size;
//	size = get_file();
	size = get_lines();

	hexprint(0,size);
	buffer[size] = EOS;
	print_short_vars();
	print_vars();
	printf("size=%d\n", size);
	return 0;
}

uint32_t get_file_size(filedsc_t *f)
{
	struct stat st;
	if (f->name == NULL) {
		f->size = 0;
	} else {
		stat(f->name, &st);
		f->size = st.st_size;
	}
	return f->size;
}


void get_infile(void)
{
}

void init(void)
{
	infile.data =	NULL;
	infile.file =	stdin;
	infile.fd =		0;
	infile.name =	NULL;	// "stdin";
	infile.size =	1000000;
	infile.open	=	1;

	outfile.data =	NULL;
	infile.file =	stdout;
	outfile.fd =	1;
	outfile.name = 	NULL;	// "stdout";
	outfile.size =	0;
	outfile.open =	1;

	editfile.data =	NULL;
	infile.file =	NULL;
	editfile.fd =	-1;
	editfile.name = "";
	editfile.size =	0;
	editfile.open =	1;
}

void fileinfo(char *fname, filedsc_t *f)
{
	printf("%s (%s): %d\n", f->name, fname, f->size);
}

void open_outfile(filedsc_t *of)
{
	FILE *f;
	if (of->open == 1) {
		/* No file, We use stdout */
	} else {
		f = fopen(of->name,"w");
		if (f == NULL) {
			fprintf(stderr, "Error opening %s for writing\n", of->name);
			exit(1);
		}
		of->file = f;
	}
}

void open_infile(filedsc_t *in)
{
	FILE *f;
	f = fopen(in->name,"r");
	if (f == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", in->name);
		exit(1);
	}
	in->file = f;
}


int main (int argc, char **argv)
{
	int i, size, count;
	int remaining;
	int c;
	init();

	while ((c = getopt (argc, argv, "f:o:")) != -1) {
		switch (c) {
		case 'f':
			editfile.name = optarg;
			editfile.open = 0;
			break;
		case 'o':
			outfile.name = optarg;
			outfile.open = 0; 
			break;
		case '?':
			if (optopt == 'f')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr,
					"Unknown option character `\\x%x'.\n",
				optopt);
				return 1;
		default:
			fprintf(stderr, "Bad command line\n");
			return 1;
		}
	}
	remaining = argc - optind;
	if (remaining == 1) {
		infile.name = argv[optind];
		infile.open = 0;
	} else if (remaining > 1) {
		printf ("Too many arguments %s\n", argv[optind]);
		return 1;
	}
	get_file_size(&infile);
	get_file_size(&outfile);
	get_file_size(&editfile);
	printf("==================\n");
	fileinfo("infile",  &infile);
	fileinfo("outfile", &outfile);
	fileinfo("editfile", &editfile);
	printf("==================\n");
	return 0;
}


