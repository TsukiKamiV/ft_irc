NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp \
		Channel.cpp \
		Client.cpp \
		Commands.cpp \
		Mode.cpp \
		ServerAuth.cpp \
		ServerChannel.cpp \
		ServerCore.cpp \
		ServerIO.cpp

OBJS = $(SRCS:.cpp=.o)

INCLUDES = Server.hpp Client.hpp Channel.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
