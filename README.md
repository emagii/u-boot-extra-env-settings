# u-boot-extra-env-settings
Create the U-Boot CONFIG_EXTRA_ENV_SETTINGS from a text file

Each variable is separated by an empty line.
Indent the variable as you like it to be indented using <space> characters
A simple pretty printer in u-boot would then allow nice printouts.

The end result can then be included in "include/configs/<myheader>.h"

Possible switches in an extension

    -v|--variable  <variable>   name instead of CONFIG_EXTRA_ENV_SETTINGS
    -f|--file   <file>          read text file instead of stdin
    -o|--output <file>          write the result to a file
    -x|--extend <file>          add the result to the end of a file.

Right now you do:

    cat <text-file> | genenv > <output-file>


I.E:
    cat textenv.txt | ./genenv > textenv.env
