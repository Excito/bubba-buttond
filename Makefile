CFLAGS=-Os -Wall

APP=buttond
APP_SRC=main.c


OBJS=$(APP_SRC:%.c=%.o)

all: $(APP)

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

clean:                                                                          
	rm -f *~ $(APP) $(OBJS)

.PHONY: clean all pre
