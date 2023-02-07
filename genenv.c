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

#define	MAX_LINES	  0x10000
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

extern	char *buffer;
extern int32_t get_input(char *filename);
typedef	char *string;

typedef struct {
	int	pos;		/* Start in buffer for this variable */
	int last;		/* Start in buffer for next variable */
	int valid;		/* Has been defined */
	int line;
	char *name;
} var_t;

typedef struct {
	char		*data;
	FILE		*file;
	int32_t		fd;
	char		*name;
	uint32_t	size;
	uint32_t	open;
} filedsc_t;

uint32_t	pos[MAX_LINES];
string	lines[MAX_LINES];
uint32_t	linecount;
var_t		variables[MAX_LINES];
uint32_t	varcount;

filedsc_t	infile;		/* We read the instructions from here */
filedsc_t	outfile;	/* Result */
filedsc_t	editfile;	/* We insert the result here, if present */

char *get_string(uint32_t i)
{
	return &buffer[pos[i]];
}

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

void fprint_var(FILE *fd, uint32_t v)
{
	uint32_t line;
	char *p;
	uint32_t done;
	uint32_t skip=0;
	fprintf(fd, "#define %s = \\\n", variables[v].name);
	line = variables[v].line + 1;
	done = 0;
	do {
		p = get_string(line);
		if (p[0] == '#') {
			if (p[1] == '!') {	/* END of variable */ 
				done = 1;
				if (skip == 0) {
					fprintf(fd, "\t\"\\0\" \\\n\n");
					skip = 1;
				} else {
					fprintf(fd, "\n");
				}
			} else {			/* COMMENT */
				line++;
				continue;
			}
		} else if (p[0] == '\0') {	/* Empty line */ 
			if (p[1] == '\0') {	/* END of variable */ 
				if (skip == 0) {
					fprintf(fd, "\t\"\\0\" \\\n");
				}
				skip = 1;
				line++;
			} else {
				if (skip == 0) {
					fprintf(fd, "\t\"\\0\" \\\n");
				}
				skip = 1;
			}
		} else {
			skip = 0;
			fprintf(fd, "\t\"%s\" \\\n", p);
		}
		line++;
	} while(!done);
}

uint32_t scan_vars(void)
{
	char *line, *p;
	uint32_t var_ix = 0;
	for (uint32_t l = 0 ; l < linecount ; l++) {
		line = lines[l];
		if ((line[0] == '#') && (line[1] == '!')) {
			uint32_t var_pos = pos[l] + 2;
			variables[var_ix].pos = var_pos;		/* Identify the buffer position for the variable */
			variables[var_ix].line = l;
			p = &buffer[var_pos];
			variables[var_ix].name = p;
			variables[var_ix].valid = 1;
			while (idchar(*p)) {
				p++;
			}
			*p = '\0';
			var_ix++;
		}
	}
	return var_ix;
}

uint32_t _get_lines(void)
{
	uint32_t	i = 0;
	for(uint32_t line = 0 ; line < MAX_LINES ; line++) {
		pos[line] = i;
		while(buffer[i] != LF) {
			if (buffer[i] == '\0') {
				return line;
			}
			i++;
		}
		buffer[i++] = '\0';
	}
	fprintf(stderr, "Line buffer too small\n");
	return 0;	
}

uint32_t get_lines(void)
{
		uint32_t l = _get_lines();
		for (uint32_t i = 0 ; i < l ; i++) {
			lines[i] = get_string(i);
		}
		return l;
}

uint32_t get_file_size(filedsc_t *f)
{
	struct stat st;
	int status;
	if (f->name == NULL) {
		f->size = 0;
	} else {
		status = stat(f->name, &st);
		if (status == 0) {
			f->size = st.st_size;
		}
	}
	return f->size;
}

void fileinfo(char *fname, filedsc_t *f)
{
	printf("%-30s (%s): %d\n", f->name, fname, f->size);
}

void init(void)
{
	infile.data =	NULL;
	infile.file =	stdin;
	infile.fd =		0;
	infile.name =	NULL;
	infile.size =	1000000;
	infile.open	=	1;

	outfile.data =	NULL;
	outfile.file =	stdout;
	outfile.fd =	1;
	outfile.name = 	NULL;
	outfile.size =	0;
	outfile.open =	1;

	editfile.data =	NULL;
	editfile.file =	NULL;
	editfile.fd =	-1;
	editfile.name = "none";
	editfile.size =	0;
	editfile.open =	1;
}

int main (int argc, char **argv)
{
	int i, size, count;
	int remaining;
	int c;
	FILE *of;
	init();

	of = stdout;

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
	infile.size = get_input(infile.name);
	if (infile.size == -1) {
		fprintf(stderr, "Exiting...\n");
		return -1;
	}
	if (outfile.name != 0) {
		of = fopen(outfile.name, "w");
		if (of == NULL) {
			fprintf(stderr, "Failed to open output file: %s\n", outfile.name);
			exit(1);
		}
	}
	linecount = get_lines();
	varcount = scan_vars();
	for (uint32_t v = 0 ; v < varcount ; v++) {
		fprint_var(of, v);
	}
	if (outfile.name != NULL) {
		fclose(of);
	}
	return 0;
}


