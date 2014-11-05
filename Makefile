
all:
	$(MAKE) -C src

install: all
	cp src/tgrep /usr/local/bin
	mkdir /var/tgrep
	chmod 777 /var/tgrep
