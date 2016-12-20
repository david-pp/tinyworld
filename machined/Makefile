CXX      = g++
CXXFLAGS = -g -I../proto.client -I../common
LDFLAGS  = -L../common  -L../proto.client/ -lproto.client -L/usr/local/lib -lzmq -lprotobuf -ltinyworld

OBJS = machined.o

SRCS = $(OBJS:%.o=%.cpp)
DEPS = $(OBJS:%.o=.%.d) 

TARGET=gateserver

.PHONY: all clean

all : $(TARGET)

include $(DEPS)
$(DEPS): $(SRCS)
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ >$@; \
		rm -f $@.$$$$

$(OBJS): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET): $(OBJS) ../common/libtinyworld.a
	$(CXX) $(OBJS) -o $@ $(CXXFLAGS) $(LDFLAGS)

clean:
	@rm -rf $(TARGET)
