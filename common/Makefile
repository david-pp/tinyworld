CXX      = g++
CXXFLAGS = -g -Wall -g -Wall -I./ `mysql_config --include` `pkg-config --cflags liblog4cxx`
LDFLAGS  = `mysql_config --libs_r` `pkg-config --libs liblog4cxx`

OBJS = mydb.o url.o hashkit.o app.o

SRCS = $(OBJS:%.o=%.cpp)
DEPS = $(OBJS:%.o=.%.d) 

TARGET=libtinyworld.a

all : $(TARGET)

include $(DEPS)
$(DEPS): $(SRCS)
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ >$@; \
		rm -f $@.$$$$

$(OBJS): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET) : $(OBJS)
	@rm -rf $(TARGET)
	@ar cru $(TARGET) $(OBJS)
	@ranlib $(TARGET)

clean:
	@rm -rf $(TARGET) *.o
