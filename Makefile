
NAME = a.out

CC = cc
CFLAGS = -Wall -Wextra -Werror
SRC = main.c
OBJ = main.o

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

debug: CFLAGS += -g -O0
debug: all
