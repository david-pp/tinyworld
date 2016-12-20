CXX = g++

PROTOS_CXXFLAGS = -I../../../common/
PROTOS_LDFLAGS = -L../../../common/protos -lproto.client

CXXFLAGS = -g -Wall -I../ $(PROTOS_CXXFLAGS) `pkg-config --cflags liblog4cxx` `pkg-config --cflags apr-1` `pkg-config --cflags apr-util-1` `pkg-config --cflags tinyworld-boost` `pkg-config --cflags protobuf`
LDFLAGS  = $(PROTOS_LDFLAGS) `pkg-config --libs liblog4cxx` `pkg-config --libs apr-1` `pkg-config --libs apr-util-1` ` pkg-config --libs tinyworld-boost` `pkg-config --libs protobuf`


OBJS =  connection.o \
		connection_manager.o \
		session.o \
		acceptor.o \
		io_service_pool.o \
		worker_pool.o \
		client.o \
		connector.o 

SRCS = $(OBJS:%.o=%.cpp)
DEPS = $(OBJS:%.o=.%.d) 

TARGET=libtinyserver.a

.PHONY: all clean message client server

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

message : test_message.cpp
	$(CXX) -o $@ $(CXXFLAGS) test_message.cpp  $(LDFLAGS) 

server : test_server.cpp $(TARGET)
	$(CXX) -o $@ $(CXXFLAGS) test_server.cpp -L./ -ltinyserver $(LDFLAGS)

client : test_client.cpp $(TARGET)
	$(CXX) -o $@ $(CXXFLAGS) test_client.cpp -L./ -ltinyserver $(LDFLAGS) 

clean:
	@rm -rf $(TARGET) *.o client server


