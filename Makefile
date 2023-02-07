CC = gcc
TEXTENV=textenv
APP=genenv

all:	$(APP)

test:	$(TEXTENV).env
	printf "\033c"
	@cat	$<

$(APP):	$(APP).c
	@$(CC) -g -o $@ $<

$(TEXTENV).env:	$(TEXTENV).txt $(APP)
	@cat $< | ./$(APP) > $@

clean:
	@rm -fr $(APP) *.env
