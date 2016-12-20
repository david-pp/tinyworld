
#
# liblog4cxx
#
LOG4CXX_CXXFLAGS=`pkg-config --cflags liblog4cxx` `pkg-config --cflags apr-1` `pkg-config --cflags apr-util-1`
LOG4CXX_LDFLAGS=`pkg-config --libs liblog4cxx` `pkg-config --libs apr-1` `pkg-config --libs apr-util-1`

#
# mysql++
#
MYSQL_CXXFLAGS=`mysql_config --include` `pkg-config --cflags mysql++`
MYSQL_LDFLAGS=`mysql_config --libs_r` `pkg-config --libs mysql++`

#
# boost
#
BOOST_CXXFLAGS=`pkg-config --cflags tinyworld-boost`
BOOST_LDFLAGS=`pkg-config --libs tinyworld-boost`


#
# 编译/链接选项
#
CXX      = g++
CXXFLAGS = -I../common $(LOG4CXX_CXXFLAGS) $(MYSQL_CXXFLAGS) $(BOOST_CXXFLAGS)
LDFLAGS  = -L../common -ltinyworld $(LOG4CXX_LDFLAGS) $(MYSQL_LDFLAGS) $(BOOST_LDFLAGS)

TWLIB=../common/libtinyworld.a

TARGETS=mydb endian

#mycache2 myredis sql test mycache mycached yaml url test_logger test_xml 

all : $(TARGETS)

mydb : mydb.cpp $(TWLIB)
	$(CXX) -o $@ mydb.cpp $(CXXFLAGS) $(LDFLAGS)

endian : endian.cpp
	$(CXX) -o $@ endian.cpp $(CXXFLAGS) $(LDFLAGS)


clean:
	@rm -f *.o $(TARGETS)

#
#
# 下面的尚未整理，暂时无视
#
#

LOG4CXXLIBS=-L/usr/local/lib -llog4cxx -L/usr/lib -lapr-1 -laprutil-1 
#LOG4CXXLIBS=-I/opt/local/include -L/opt/local/lib -llog4cxx -lapr-1 -laprutil-1 
MYSQLFLAGS=-I/usr/local/include/mysql++ -I/usr/include/mysql -I/user/local/mysql/include -lmysqlpp
#MYSQLFLAGS=`mysql_config --cxxflags` `mysql_config --libs_r`  -lmysqlpp
PROTOFLAGS=-I../proto.server -L../proto.server/ -lproto.server -lprotobuf
ZMQFLAGS=-lzmq
BOOSTFLAGS= -lboost_thread-mt 
#BOOSTFLAGS= -lboost_thread -lboost_system
YAMLFLAGS=-L/usr/local/lib -lyaml-cpp

#
# Redis
#
REDISFLAGS=-lhiredis -lev
REDIS_CPP=../common/myredis.cpp ../common/myredis_sync.cpp ../common/myredis_pool.cpp ../common/myredis_async.cpp ../common/url.cpp ../common/hashkit.cpp ../common/callback.cpp ../common/eventloop.cpp
REDIS_HEADER=../common/myredis.h ../common/myredis_pool.h ../common/myredis_sync.h ../common/myredis_async.h


test_xml.h : test_xml.schema.xml ../tools/xmlpg.py
	@../tools/xmlpg.py --cpp --header -i test_xml.schema.xml > test_xml.h
test_xml : test_xml.cpp test_xml.h	
	$(CXX) -o $@ test_xml.cpp  $(CXXFLAGS) $(LDFLAGS)


test_logger : test_logger.cpp
	$(CXX) -o $@ $< -I/usr/local/include $(LOG4CXXLIBS)

test_redis : test_redis.cpp ../common/myredis.cpp ../common/myredis.h
	$(CXX) -g -o $@ test_redis.cpp ../common/myredis.cpp ../common/url.cpp ../common/hashkit.cpp -I/usr/local/include -L/usr/local/lib -lhiredis $(BOOSTFLAGS) $(LOG4CXXLIBS)

redisasync : redisasync.cpp ../common/redis_async.cpp ../common/redis_async.h ../common/callback.h
	$(CXX) -g -o $@ redisasync.cpp ../common/redis_async.cpp $(REDIS_CPP) -I/usr/local/include -L/usr/local/lib -lhiredis -lev  $(LOG4CXXLIBS) $(BOOSTFLAGS)

cache : cache.cpp ../common/mycache.cpp ../common/mycache.h
	$(CXX) -g -o $@ cache.cpp  $(REDIS_CPP) ../common/mydb.cpp ./common/mycache.cpp -I/usr/local/include/mysql++ -I/usr/include/mysql -L/usr/local/lib -lhiredis -lmysqlpp -lzmq  $(LOG4CXXLIBS)

url : ../common/url.h ../common/url.cpp
	$(CXX) -g -o $@ -D_TEST_FOR_URL -Wall ../common/url.cpp

MYCACHE_CPP= mycache.cpp ../common/mycache.cpp ../common/callback.cpp
mycache :  $(MYCACHE_CPP) ../common/mycache.h ../common/pool.h ../common/callback.h
	$(CXX) -g -o $@ -Wall  $(MYCACHE_CPP)  $(PROTOFLAGS) $(ZMQFLAGS) $(BOOSTFLAGS) $(MYSQLFLAGS) $(LOG4CXXLIBS)

MYCACHED_CPP=../common/mycached.cpp ../common/mycache.cpp ../common/mydb.cpp $(REDIS_CPP)
mycached : ../common/mycached.cpp ../common/mycached.h 
	$(CXX) -g -o $@ -D_ONLY_FOR_TEST -Wall $(MYCACHED_CPP) $(LOG4CXXLIBS) $(PROTOFLAGS) $(ZMQFLAGS) $(REDISFLAGS) $(BOOSTFLAGS) $(YAMLFLAGS) $(MYSQLFLAGS)

test : test.cpp
	$(CXX) -g -o $@ $< -Wall $(BOOSTFLAGS) $(ZMQFLAGS)


myredis : myredis.cpp $(REDIS_CPP) $(REDIS_HEADER)
	$(CXX) -g -o $@ myredis.cpp $(REDIS_CPP) $(BOOSTFLAGS) $(REDISFLAGS) $(LOG4CXXLIBS)

mycache2 : mycache2.cpp ../common/mycache2.h ../common/mycache2.cpp $(REDIS_CPP) $(REDIS_HEADER)
	$(CXX) -g -o $@ mycache2.cpp ../common/mycache2.cpp $(REDIS_CPP) $(BOOSTFLAGS) $(REDISFLAGS) $(LOG4CXXLIBS)

yaml : yaml.cpp
	$(CXX) -g -o $@ yaml.cpp $(YAMLFLAGS)

