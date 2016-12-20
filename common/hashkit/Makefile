CXX      = gcc
CXXFLAGS = -I./
LDFLAGS  = 

.PHONY : test clean

SRCS = nc_crc16.c	\
	nc_crc32.c		\
	nc_fnv.c		\
	nc_hsieh.c		\
	nc_jenkins.c	\
	nc_ketama.c		\
	nc_md5.c		\
	nc_murmur.c		\
	nc_one_at_a_time.c \
	hashkit.c

OBJS = $(SRCS:%.c=%.o)
DEPS = $(SRCS:%.c=.%.d) 

TARGET=libhashkit.a

all : $(TARGET)

include $(DEPS)
$(DEPS): $(SRCS)
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ >$@; \
		rm -f $@.$$$$

$(OBJS): %.o: %.c
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET) : $(OBJS)
	@rm -rf $(TARGET)
	@ar cru $(TARGET) $(OBJS)
	@ranlib $(TARGET)

clean:
	@rm -rf $(TARGET) *.o

test:
	@g++ -g -o test test.cpp -L./ -lhashkit