CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./src
AR       = ar
ARFLAGS  = rcs

OBJ_DIR  = obj
LIB_SRCS = src/App.cpp
LIB_OBJS = $(LIB_SRCS:%.cpp=$(OBJ_DIR)/%.o)

EXEC	 = example.out

TARGET   = libclap.a

all: $(TARGET)

$(TARGET): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(TARGET) $(EXEC)

re: fclean all

compile: all
	$(CXX) $(CXXFLAGS) -o $(EXEC) main.cpp -L. -lclap

.PHONY: all clean fclean re
