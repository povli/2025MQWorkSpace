# Variables
CXX := g++
CXXFLAGS := -std=c++11 -O2 -g -Iinclude -Isrc/common -Isrc/server -pthread
LDFLAGS := -lmuduo_net -lmuduo_base -lpthread -lprotobuf

SRC_DIR := src
SERVER_SRC := $(wildcard src/server/*.cpp)
CLIENT_SRC := $(wildcard src/client/*.cpp)
TEST_SRC := $(wildcard test/*.cpp)
PROTO_SRCS := src/common/msg.proto src/common/protocol.proto

# Targets
all: server client test

# Compile server
server: $(SERVER_SRC:.cpp=.o) $(wildcard src/server/*.hpp) src/common/msg.pb.o src/common/protocol.pb.o
	$(CXX) $(CXXFLAGS) -o mq_server $(SERVER_SRC:.cpp=.o) src/common/msg.pb.o src/common/protocol.pb.o $(LDFLAGS)

# Compile client
client: $(CLIENT_SRC:.cpp=.o) src/common/msg.pb.o src/common/protocol.pb.o
	$(CXX) $(CXXFLAGS) -o mq_client $(CLIENT_SRC:.cpp=.o) src/common/msg.pb.o src/common/protocol.pb.o $(LDFLAGS)

# Compile test (with Google Test framework)
test: $(TEST_SRC:.cpp=.o) $(SERVER_SRC:.cpp=.o) src/common/msg.pb.o src/common/protocol.pb.o
	$(CXX) $(CXXFLAGS) -o mq_test $(TEST_SRC:.cpp=.o) $(SERVER_SRC:.cpp=.o) src/common/msg.pb.o src/common/protocol.pb.o $(LDFLAGS) -lgtest -lgtest_main

# Pattern rules for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generate protobuf sources
src/common/msg.pb.cc src/common/msg.pb.h: src/common/msg.proto
	protoc -Isrc/common --cpp_out=src/common src/common/msg.proto

src/common/protocol.pb.cc src/common/protocol.pb.h: src/common/protocol.proto
	protoc -Isrc/common --cpp_out=src/common src/common/protocol.proto

# Compile protobuf objects
src/common/msg.pb.o: src/common/msg.pb.cc
	$(CXX) $(CXXFLAGS) -c src/common/msg.pb.cc -o $@

src/common/protocol.pb.o: src/common/protocol.pb.cc
	$(CXX) $(CXXFLAGS) -c src/common/protocol.pb.cc -o $@

# Clean target
clean:
	rm -f mq_server mq_client mq_test \
          src/common/*.pb.cc src/common/*.pb.h \
          src/common/*.o src/server/*.o src/client/*.o test/*.o

.PHONY: all clean server client test
