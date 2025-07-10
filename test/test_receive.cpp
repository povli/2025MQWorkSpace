#include <gtest/gtest.h>
#include "../server/virtual_host.hpp"
#include "../server/consumer.hpp"

using namespace hz_mq;

static void push_once(const virtual_host::ptr& host,
                      const consumer_manager::ptr& cmp,
                      const std::string& qname)
{
    message_ptr mp = host->basic_consume(qname);
    if (!mp) return;
    consumer::ptr cp = cmp->choose(qname);
    if (!cp) return;
    cp->callback(cp->tag, mp->mutable_payload()->mutable_properties(), mp->payload().body());
    if (cp->auto_ack)
        host->basic_ack(qname, mp->payload().properties().id());
}

class ReceiveFixture : public ::testing::Test {
protected:
    void SetUp() override {
        host = std::make_shared<virtual_host>("vh","./data","./tmp.db");
        cmp  = std::make_shared<consumer_manager>();
        ASSERT_TRUE(host->declare_queue("q1", false,false,false,{}));
        cmp->init_queue_consumer("q1");
    }
    virtual_host::ptr host;
    consumer_manager::ptr cmp;
};

TEST_F(ReceiveFixture, PullSingleMessage) {
    BasicProperties bp; bp.set_routing_key("q1");
    ASSERT_TRUE(host->basic_publish("q1", &bp, "one"));
    auto msg = host->basic_consume("q1");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->payload().body(), "one");
    host->basic_ack("q1", msg->payload().properties().id());
    EXPECT_EQ(host->basic_consume("q1"), nullptr);
}

TEST_F(ReceiveFixture, PullFromEmptyQueue) {
    EXPECT_EQ(host->basic_consume("q1"), nullptr);
}

TEST_F(ReceiveFixture, PushAutoAck) {
    std::string received;
    auto cb = [&](const std::string&, const BasicProperties*, const std::string& body){
        received = body;
    };
    cmp->create("tag1","q1",true,cb);
    BasicProperties bp; bp.set_routing_key("q1");
    host->basic_publish("q1", &bp, "hello");
    push_once(host, cmp, "q1");
    EXPECT_EQ(received, "hello");
    EXPECT_EQ(host->basic_consume("q1"), nullptr);
}

TEST_F(ReceiveFixture, PushRoundRobin) {
    int c1=0,c2=0;
    auto cb1 = [&](const std::string&, const BasicProperties*, const std::string&){ ++c1; };
    auto cb2 = [&](const std::string&, const BasicProperties*, const std::string&){ ++c2; };
    cmp->create("t1","q1",true,cb1);
    cmp->create("t2","q1",true,cb2);
    BasicProperties bp; bp.set_routing_key("q1");
    for(int i=0;i<4;++i){
        host->basic_publish("q1", &bp, "m");
        push_once(host, cmp, "q1");
    }
    EXPECT_EQ(c1+c2, 4);
    EXPECT_TRUE(std::abs(c1-c2) <= 1); // approximate round-robin
}

TEST_F(ReceiveFixture, PullFifoOrder) {
    BasicProperties bp; bp.set_routing_key("q1");
    host->basic_publish("q1", &bp, "a");
    host->basic_publish("q1", &bp, "b");
    host->basic_publish("q1", &bp, "c");
    auto m1 = host->basic_consume("q1");
    auto m2 = host->basic_consume("q1");
    auto m3 = host->basic_consume("q1");
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    ASSERT_NE(m3, nullptr);
    EXPECT_EQ(m1->payload().body(), "a");
    EXPECT_EQ(m2->payload().body(), "b");
    EXPECT_EQ(m3->payload().body(), "c");
}

TEST_F(ReceiveFixture, PushNoConsumerDrop) {
    BasicProperties bp; bp.set_routing_key("q1");
    host->basic_publish("q1", &bp, "lost");
    push_once(host, cmp, "q1");
    EXPECT_EQ(host->basic_consume("q1"), nullptr);
}

TEST_F(ReceiveFixture, PushCallbackProperties) {
    std::string tag, body, rkey;
    auto cb = [&](const std::string& tg, const BasicProperties* props, const std::string& b){
        tag = tg; body = b; if (props) rkey = props->routing_key();
    };
    cmp->create("ctag","q1",true,cb);
    BasicProperties bp; bp.set_routing_key("prop");
    host->basic_publish("q1", &bp, "data");
    push_once(host, cmp, "q1");
    EXPECT_EQ(tag, "ctag");
    EXPECT_EQ(body, "data");
    EXPECT_EQ(rkey, "prop");
}

TEST_F(ReceiveFixture, PullThenPushSequence) {
    BasicProperties bp; bp.set_routing_key("q1");
    host->basic_publish("q1", &bp, "first");
    auto m = host->basic_consume("q1");
    ASSERT_NE(m, nullptr);
    host->basic_ack("q1", m->payload().properties().id());
    std::string got;
    auto cb = [&](const std::string&, const BasicProperties*, const std::string& b){ got = b; };
    cmp->create("t","q1",true,cb);
    host->basic_publish("q1", &bp, "second");
    push_once(host, cmp, "q1");
    EXPECT_EQ(got, "second");
}

