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

SOURCES=$(APP_SRC) $(APP2_SRC)

OBJS=$(APP_SRC:%.c=%.o)

APP2=write-magic
APP2_SRC=write-magic.c
OBJS2=$(APP2_SRC:%.c=%.o)

DEPDIR = .deps

all: pre $(APP) $(APP2)

%.o : %.c
	$(COMPILE.c) $(CFLAGS_EXTRA) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po

-include $(SOURCES:%.c=.deps/%.Po)

pre:
	@@if [ ! -d .deps ]; then mkdir .deps; fi

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@

$(APP2): $(OBJS2)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@


clean:                                                                          
	rm -f *~ $(APP) $(APP2) $(OBJS) $(OBJS2)
	rm -rf .deps

.PHONY: clean all pre
