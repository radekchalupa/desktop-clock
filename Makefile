CFLAGS = -O2 -Wall -pedantic -std=c99 `pkg-config --cflags gdk-3.0`
LDFLAGS = `pkg-config --libs gdk-3.0` -lX11

objects = desktop-clock.o
target = rc-desktop-clock

%.o : %.c config.h
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<

$(target) : $(objects)
	$(CC) $(CFLAGS) $(objects) $(LDFLAGS) -o $(target) 

.PHONY : clean
clean: 
	rm -f $(objects) $(target)
