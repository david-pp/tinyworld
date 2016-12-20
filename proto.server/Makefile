
CXX=g++
CXXFLAGS=

PROTOC = protoc --cpp_out=.
PROTOLIB = libproto.server.a
PROTOLIB_SO = libproto.server.so

PROTOFILES = mycache.proto 

PROTO_HS   = $(PROTOFILES:%.proto=%.pb.h)
PROTO_CCS  = $(PROTOFILES:%.proto=%.pb.cc)
PROTO_OBJS = $(PROTOFILES:%.proto=%.pb.o)

.PHONY: all clean test

all : $(PROTOLIB)

$(PROTO_CCS) : %.pb.cc : %.proto
	$(PROTOC) $<

$(PROTO_OBJS) : %.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@ 

$(PROTOLIB) : $(PROTO_OBJS)
	@rm -rf $(PROTOLIB)
	@ar cru $(PROTOLIB) $(PROTO_OBJS)
	@ranlib $(PROTOLIB)

clean:
	@rm -rf $(PROTOLIB) *.pb.h *.pb.cc *.pb.o


test:
	$(CXX) -o test test.cpp  -L./  -L/usr/local/lib -lproto.client -lprotobuf