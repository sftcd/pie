# XML2RFC=/Users/paul/Documents/xml2rfc-1.35/xml2rfc.tcl
XML2RFC=xml2rfc
CC=g++ -g

all: draft tool

draft:
	$(XML2RFC) draft-farrell-pie.xml draft-farrell-pie-00.txt

tool: 
	$(CC) pie.cc -o pie -l crypto

clean:
	rm -f   draft-farrell-pie-00.txt pie *~

