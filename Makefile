CC = gcc
TEXTENV=textenv
APP=genenv

all:	$(APP)

test:	$(TEXTENV).env.1
	printf "\033c"
	@cat	$<

test2:	$(TEXTENV).env.2
	printf "\033c"
	@cat	$<

$(APP):	$(APP).o get_input.o
	@$(CC) -g -o $@ get_input.o $(APP).o

$(APP).o:	$(APP).c
	@$(CC) -g -c $<

get_input.o:	get_input.c
	@$(CC) -g -c $<

$(TEXTENV).env.1:	$(TEXTENV).txt $(APP)
	@cat $< | ./$(APP) > $@

$(TEXTENV).env.2:	$(TEXTENV).txt $(APP)
	@./$(APP) -f machine.h -o $@ textenv.txt

clean:
	@rm -fr $(APP) *.env*
