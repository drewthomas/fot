all: fot fot.ps fot.1.gz

fot: fot.c
	gcc -ansi -std=c99 -pedantic -s -Os -Wall -Wextra fot.c -o fot
#	gcc -ansi -std=c99 -pedantic -g -pg -Os -Wall -Wextra fot.c -o fot

fot.ps fot.1.gz: fot.xml
	docbook2x-man fot.xml
	groff -man fot.1 > fot.ps
	gzip -9 fot.1

install: fot fot.ps fot.1.gz
	install fot /usr/local/bin/
	cp fot.1.gz /usr/share/man/man1

uninstall:
	rm -f /usr/local/bin/fot /usr/share/man/man1/fot.1.gz
