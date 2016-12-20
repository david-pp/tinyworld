#
# compiler
#
export CXX = g++

#
# liblog4cxx
#
export LOG4CXX_CXXFLAGS=`pkg-config --cflags liblog4cxx` `pkg-config --cflags apr-1` `pkg-config --cflags apr-util-1`
export LOG4CXX_LDFLAGS=`pkg-config --libs liblog4cxx` `pkg-config --libs apr-1` `pkg-config --libs apr-util-1`

#
# mysql++
#
export MYSQL_CXXFLAGS=`mysql_config --include` `pkg-config --cflags mysql++`
export MYSQL_LDFLAGS=`mysql_config --libs` `pkg-config --libs mysql++`

#
# boost
#
export BOOST_CXXFLAGS=`pkg-config --cflags tinyworld-boost`
export BOOST_LDFLAGS=`pkg-config --libs tinyworld-boost`

#
# protobuf
#
export PROTOBUF_CXXFLAGS=`pkg-config --cflags protobuf`
export PROTOBUF_LDFLAGS=`pkg-config --libs protobuf`


DIRS =  ../common/protos \
		protos \
		common/net_asio \
		common \
		test \
		gateserver \
		worldmanager \
		worldserver \
		loginserver \
		client

.PHONY : all login client gate worldm world clean distclean

all : 
	@for dir in $(DIRS); do \
		make -C $$dir || exit 1; \
	done

login :
	make -C loginserver clean
	make -C loginserver

gate :
	make -C gateserver clean
	make -C gateserver

client :
	make -C client clean
	make -C client

worldm :
	make -C worldmanager clean
	make -C worldmanager

world :
	make -C worldserver clean
	make -C worldserver


clean :
	@for dir in $(DIRS); do \
		make -C $$dir clean; \
	done

distclean:
	@find . -iname .\*.d -exec rm \{\} \;
	@find . -iname .\*.d.* -exec rm \{\} \;
