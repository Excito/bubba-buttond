CFLAGS_EXTRA=\
			 -O3 \
			 -g \
			 -Wall \
			 -Wextra \
			 -Wno-unused-result \
			 -DPIDFILE="\"/var/run/bubba-buttond.pid\"" \
			 -DREBOOTCMD="\"/sbin/reboot\"" \
			 -DDEVICE="\"/dev/input/by-path/platform-gpio-keys-event\"" \
			 -DMTD_PART="\"/dev/mtd2\""
LDFLAGS_EXTRA=

APP=buttond
APP_SRC=main.c

SOURCES=$(APP_SRC)

OBJS=$(APP_SRC:%.c=%.o)

DEPDIR = .deps

%.o : %.c
	$(COMPILE.c) $(CFLAGS_EXTRA) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

-include $(SOURCES:%.c=.deps/%.Po)

all: pre $(APP)

pre:
	@@if [ ! -d .deps ]; then mkdir .deps; fi

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@

clean:                                                                          
	rm -f *~ $(APP) $(OBJS)
	rm -rf .deps

.PHONY: clean all pre
