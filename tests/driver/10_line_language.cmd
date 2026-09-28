# The line language of the driver: comments, empty lines, whitespace, separators.
#!exit 0
# a comment line
# an empty line follows this one

show 7/3
   # an indented comment line
   show 7/3

# a line of whitespace only
	 

show 7/3
# a tab between the operation and the operand
show	7/3
# trailing whitespace after the operand
show 7/3   
# the separator is the word "with" between spaces (README of tools/adf)
add 7/3 with 1/3
