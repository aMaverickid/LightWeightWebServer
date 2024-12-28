# 定义变量
CC = g++
CFLAGS = -Wall -O2
INCLUDES = -Ifastcgi
TARGET = server
OBJS = server.o utils.o fastcgi/fastcgi.o

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 生成目标文件
server.o: server.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

utils.o: utils.cpp
	$(CC) $(CFLAGS) -c $< -o $@

fastcgi/fastcgi.o: fastcgi/fastcgi.c
	$(CC) $(CFLAGS) $(INCLUDES) -x c -c $< -o $@

# 清理目标文件和中间文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean
