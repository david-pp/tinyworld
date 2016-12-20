CXXFLAGS = -g -Wall -I../common -I../  -I../../common $(LOG4CXX_CXXFLAGS) $(MYSQL_CXXFLAGS) $(BOOST_CXXFLAGS) $(PROTOBUF_CXXFLAGS)
LDFLAGS  = -L../common -L../common/net_asio -L../protos  -L../../common/protos -ltinyworld -ltinyserver -lproto.client -lproto.server  $(LOG4CXX_LDFLAGS) $(MYSQL_LDFLAGS) $(BOOST_LDFLAGS) $(PROTOBUF_LDFLAGS)

TINYWORLDLIBS=../common/libtinyworld.a ../common/net_asio/libtinyserver.a


OBJS = main.o gateserver.o 

SRCS = $(OBJS:%.o=%.cpp)
DEPS = $(OBJS:%.o=.%.d) 

TARGET=gateserver

.PHONY: all clean distclean

all : $(TARGET)

include $(DEPS)
$(DEPS): $(SRCS)
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ >$@; \
		rm -f $@.$$$$

$(OBJS): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET): $(OBJS) $(TINYWORLDLIBS)
	$(CXX) -o $@  $(OBJS)  $(LDFLAGS)

clean:
	@rm -rf $(TARGET) *.o

distclean:
	@rm -rf .*.d