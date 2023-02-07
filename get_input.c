/*
 * gen_input.c
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

char	*buffer = NULL;
int32_t	 filesize;

#define STDIN_SIZE  10000000

char *alloc_buffer(int32_t size)
{
	char *buf;
	buf = malloc(size);
	if (buf == NULL) {
		fprintf(stderr, "Failed to allocate %d bytes\n", size);
	}
	return buf;
}

int32_t get_input(char *filename)    /* if NULL, we get stdin */
{
	FILE *fp;
	char c;
	if (filename == NULL) {	/* stdin */
		buffer = alloc_buffer(sizeof(char) * STDIN_SIZE);
		if (buffer == NULL) {
			return -1;
		}
		for (int i = 0 ; i < STDIN_SIZE-2 ; i++) {
			c = getchar();
			if (c == EOF) {
				filesize = i;
				buffer[i  ] = '\0';
				buffer[i+1] = '\0';
#if	0
				for (int32_t j = 0; j <= i ; j++) {
					printf("0x%02X ", buffer[j]);
				}
				printf("\n");
#endif
				return i;
			}
			buffer[i] = c;
		}
		fprintf(stderr, "File >= 1 MB\n");
		return -1;		/* file too large */
	} else {
		fp = fopen(filename,"r");
		if (fp == NULL) {
			fprintf(stderr, "Error opening %s for reading\n", filename);
			return -1;
		}
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			filesize = ftell(fp);
			if (filesize == -1) {
				fprintf(stderr, "Bad file size: %s\n", filename);
				fclose(fp);
				return -1;
			}

			/* Allocate our buffer to that size. */
			buffer = alloc_buffer(sizeof(char) * (filesize + 1));
			if (buffer == NULL) {
				fclose(fp);
				return -1;
			}
			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) {
				fprintf(stderr, "Cannot read from start: %s\n", filename);
				fclose(fp);
				return -1;
			}

			/* Read the entire file into memory. */
			size_t newLen = fread(buffer, sizeof(char), filesize, fp);
			if ( ferror( fp ) != 0 ) {
				fprintf(stderr, "Error reading file: %s", filename);
				fclose(fp);
				return -1;
			} else {
				buffer[newLen++] = '\0'; /* Just to be safe. */
			}
			fclose(fp);
			return filesize;
		}
	}
}
