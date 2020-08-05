# $Id: $

all: policy
file=policy
policy:  $(file).c
	cc -I/usr/include -I/usr/local/include -I/usr/local/include/postgresql -c -o $(file).o $(file).c
	cc -static -L/usr/lib -L/usr/local/lib -L/usr/local/lib/postgresql -o $(file) $(file).o -lkcgi -lz -lpq -lpgcommon -lpgport -lxml2 -lssl -lcrypto -lz -lreadline -ltermcap -lm

check: all
	doas kfcgi -U www -u www -- /cgi-bin/policy | grep "Status: 200 OK"
	@echo "*** ALL TESTS PASSED ***"

clean:
	-rm $(file)

install:
	doas install -o www -g www -m 0500 ${file} /var/www/cgi-bin
	doas install -o www -g www -m 0444 ${file}.html /var/www/htdocs
	doas rm -f ${file}.o

.PHONY: call clean check install
