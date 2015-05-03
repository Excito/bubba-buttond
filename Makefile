CFLAGS_EXTRA=\
			 -O3 \
			 -g \
			 -Wall \
			 -Wextra \
			 -Wno-unused-result \
			 -std=gnu99 \
			 -DPIDFILE="\"/var/run/bubba-buttond.pid\"" \
			 -DHALTCMD="\"/sbin/shutdown -h now\"" \
			 -DDEVICE="\"/dev/input/by-path/platform-gpio-keys-event\""
LDFLAGS_EXTRA=

APP=bubba-buttond
APP_SRC=bubba-buttond.c

OBJ=$(APP_SRC:%.c=%.o)

SOURCES=$(APP_SRC)
APPS=$(APP)
OBJS=$(OBJ)
DEPDIR = .deps

all: pre $(APPS)

%.o : %.c
	$(COMPILE.c) $(CFLAGS_EXTRA) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

-include $(SOURCES:%.c=.deps/%.Po)

pre:
	@@if [ ! -d .deps ]; then mkdir .deps; fi

$(APP): $(OBJ)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@

clean:                                                                          
	rm -f *~ $(APPS) $(OBJS)
	rm -rf .deps

.PHONY: clean all pre
