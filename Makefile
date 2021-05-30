NAME=tetris
CC=g++
CFLAGS=-O2 -pthread -s

b:
	$(CC) $(CFLAGS) -o $(NAME) main.cpp
r:
	alacritty -e ./$(NAME)

br: b r

install: b
	sudo mkdir -p /bin
	sudo mkdir -p /usr/bin
	sudo cp -f $(NAME) /bin
	sudo cp -f $(NAME) /usr/bin
	sudo chmod 755 /bin/$(NAME)
	sudo chmod 755 /usr/bin/$(NAME)

uninstall:
	sudo rm -f /bin/$(NAME)
	sudo rm -f /usr/bin/$(NAME)
