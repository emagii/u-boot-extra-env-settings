# u-boot-extra-env-settings
Create U-Boot DEFINE's like CONFIG_EXTRA_ENV_SETTINGS from a text file

Each 'DEFINE' can add a number of environment variables.

The '#' character at the beginning of a line is the beginning of a comment.
The "#!" sequence in the beginning of a line is a DEFINE definition

I.E:
#!MY_VARIABLE

If it followed by a number of declarations of environment variables
I.E:

myvar=...

They can be multiline

myvar=
if x; then
  y;
else
  z;
fi

An environment declaration is terminated by an empty line.

You can have comments within the environment declaration,
but they are removed in the result.

Indent the variable as you like it to be indented using <space> characters
A simple pretty printer in u-boot would then allow nice printouts.

The end result can then be included in "include/configs/<myheader>.h"

Switches:
    -f|--file   <file>          insert result in this text file
                                (not yet implemented)
    -o|--output <file>          write the result to a file

Usage
$ genenv -o <output file> <input file>
$ cat <inputfile> | genenv

I.E:
    cat textenv.txt | ./genenv > textenv.env
