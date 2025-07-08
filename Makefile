# === 1. 自定义可调路径 ===
# 你的 muduo 安装路径（包含 include、lib 子目录）
MUDUO_PREFIX := src/tools/muduo/_install
EXAMPLE_INC := src/tools/muduo/examples
TOOLS_ROOT  := src/tools                 # << 新增
SRC_INC     := src/tools/muduo/muduo 


# ================= 公共变量 =================
CXX        := g++
CXXFLAGS   := -std=c++20 -O2 -g                                 \
              -Iinclude -Isrc/common -Isrc/server -Isrc/client  \
              -I$(MUDUO_PREFIX)/include                         \
			  -I$(EXAMPLE_INC) \
			  -I$(TOOLS_ROOT) \
			  -I$(SRC_INC)	\
              -pthread

# Muduo / Protobuf / 其它依赖库
LD_LIBS    := -L$(MUDUO_PREFIX)/lib \
              -lmuduo_net -lmuduo_base \
              -lprotobuf -lpthread -lz

# 源码分组
SERVER_SRC := $(wildcard src/server/*.cpp) \
              src/tools/muduo/examples/protobuf/codec/codec.cc
CLIENT_SRC := $(wildcard src/client/*.cpp) \
              src/tools/muduo/examples/protobuf/codec/codec.cc
TEST_SRC   := $(wildcard test/*.cpp)
COMMON_SRC = $(wildcard src/common/*.cpp)
COMMON_OBJS = \
    src/common/exchange.o \
    src/common/queue.o   \
    src/common/thread_pool.o \
    src/common/msg.pb.o  \
    src/common/protocol.pb.o 
             

CODEC_SRC = src/tools/muduo/examples/protobuf/codec/codec.cc


SERVER_OBJS := $(SERVER_SRC:.cpp=.o)
CLIENT_OBJS := $(CLIENT_SRC:.cpp=.o)

# Protobuf 源文件
PROTO_FILES := src/common/msg.proto src/common/protocol.proto
PROTO_CC    := $(PROTO_FILES:.proto=.pb.cc)
PROTO_HDR   := $(PROTO_FILES:.proto=.pb.h)
PROTO_OBJ   := $(PROTO_CC:.cc=.o)

# === 2. 最终目标 ===
all: mq_server mq_client mq_test

# ---------- 生成可执行文件 ----------
# -------- mq_server ------------
mq_server: $(SERVER_OBJS) $(COMMON_OBJS) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LD_LIBS)

# -------- mq_client ------------
mq_client: $(CLIENT_OBJS) $(COMMON_OBJS) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LD_LIBS)

# -------- mq_test --------------
mq_test:   $(TEST_SRC:.cpp=.o) $(SERVER_OBJS) $(COMMON_OBJS) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LD_LIBS) -lgtest -lgtest_main
# ---------- 通用规则 ----------
# 3. 先把 .cpp 编译成 .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@



# ---------- 4. Protobuf 自动生成 ----------
# 生成 *.pb.cc *.pb.h
src/common/%.pb.cc src/common/%.pb.h: src/common/%.proto
	protoc -Isrc/common --cpp_out=src/common $<



$(SERVER_SRC:.cpp=.o) $(CLIENT_SRC:.cpp=.o) $(TEST_SRC:.cpp=.o): $(PROTO_HDR)

# 将生成的 .pb.cc 编译为 .o
%.pb.o: %.pb.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------- 5. 清理 ----------
clean:
	@echo "Cleaning..."
	@rm -f mq_server mq_client mq_test \
	       $(SERVER_SRC:.cpp=.o) \
	       $(CLIENT_SRC:.cpp=.o) \
	       $(TEST_SRC:.cpp=.o) \
	       $(PROTO_CC) $(PROTO_HDR) $(PROTO_OBJ)

.PHONY: all clean
