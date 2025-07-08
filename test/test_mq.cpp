#include <gtest/gtest.h>
#include "../server/virtual_host.hpp"
#include "../server/consumer.hpp"
#include "../server/connection.hpp"       // connection_manager
#include "../common/thread_pool.hpp"      // thread_pool
#include <muduo/protoc/codec.h>  

using namespace hare_mq;

TEST(MessageQueueTest, PointToPointSendReceive) {
    // Initialize virtual host
    std::string baseDir = "./testdata";
    virtual_host vh("TestHost", baseDir, baseDir + "/meta.db");

    // Declare an exchange and queue, bind them
    ASSERT_TRUE(vh.declare_exchange("testExchange", ExchangeType::DIRECT, false, false, {}));
    ASSERT_TRUE(vh.declare_queue("queue1", false, false, false, {}));
    ASSERT_TRUE(vh.bind("testExchange", "queue1", "queue1"));

    // Publish a message
    std::string testMsg = "HelloWorld";
    BasicProperties props;
    props.set_routing_key("queue1");
    ASSERT_TRUE(vh.basic_publish("queue1", &props, testMsg));

    // Pull (consume) the message
    message_ptr msg = vh.basic_consume("queue1");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->payload().body(), testMsg);

    // Acknowledge the message
    vh.basic_ack("queue1", msg->payload().properties().id());
    // After ack, queue should have no getable messages
    EXPECT_EQ(vh.basic_query(), "");  // no message left
}

TEST(MessageQueueTest, OpenCloseChannel) {
    auto vhPtr = std::make_shared<virtual_host>("TestHost",
                                                "./data",
                                                "./data/meta.db");
    auto cmPtr = std::make_shared<consumer_manager>();

    connection_manager connMgr;

    /* 占位参数：测试阶段可以传空指针或最小线程池 */
    muduo::net::TcpConnectionPtr dummyConn;   // = nullptr
    ProtobufCodecPtr             dummyCodec;  // = nullptr
    auto                         pool = std::make_shared<thread_pool>(1);

    connMgr.new_connection(vhPtr, cmPtr, dummyCodec, dummyConn, pool);

    SUCCEED();        // 只验证不崩溃
}
