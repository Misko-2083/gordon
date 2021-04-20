CFLAGS  ?=-Wall -g -std=c11 -Wextra -Wno-sign-compare
CFLAGS  +=`pkg-config --cflags gtk+-3.0`
LDFLAGS +=`pkg-config --libs gtk+-3.0`

gordon: gordon.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f gordon

