CFLAGS_EXTRA=\
			 -O3 \
			 -g \
			 -Wall \
			 -Wextra \
			 -Wno-unused-result \
			 -std=gnu99 \
			 -DPIDFILE="\"/var/run/bubba-buttond.pid\"" \
			 -DREBOOTCMD="\"/sbin/reboot\"" \
			 -DDEVICE="\"/dev/input/by-path/platform-gpio-keys-event\"" \
			 -DMTD_PART="\"/dev/mtd2\""
LDFLAGS_EXTRA=

APP=buttond
APP_SRC=main.c


OBJ=$(APP_SRC:%.c=%.o)

APP2=write-magic
APP2_SRC=write-magic.c
OBJ2=$(APP2_SRC:%.c=%.o)

SOURCES=$(APP_SRC) $(APP2_SRC)
APPS=$(APP) $(APP2)
OBJS=$(OBJ) $(OBJ2)
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

$(APP2): $(OBJ2)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ -o $@


clean:                                                                          
	rm -f *~ $(APPS) $(OBJS)
	rm -rf .deps

.PHONY: clean all pre
