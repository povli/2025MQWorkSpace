#include <iostream>
#include <string>
#include <thread>
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpClient.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/protoc/codec.h"
#include "muduo/protoc/dispatcher.h"
#include "../common/protocol.pb.h"
#include "../common/msg.pb.h"

using namespace hare_mq;
using muduo::net::TcpConnectionPtr;

muduo::net::EventLoop g_loop;
ProtobufDispatcher g_dispatcher(std::bind([](const TcpConnectionPtr&, const MessagePtr&, muduo::Timestamp){} , 
                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
ProtobufCodecPtr g_codec;

void onMessage(const TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp ts) {
    g_codec->onMessage(conn, buf, ts);
}

// Handlers for responses from server
void onCommonResponse(const TcpConnectionPtr&, const std::shared_ptr<basicCommonResponse>& message, muduo::Timestamp) {
    std::cout << "[Response] (cid=" << message->cid() << ") OK=" << (message->ok() ? "true" : "false") << std::endl;
}
void onConsumeResponse(const TcpConnectionPtr&, const std::shared_ptr<basicConsumeResponse>& message, muduo::Timestamp) {
    std::cout << "[Message Received] consumer_tag=" << message->consumer_tag()
              << " body=\"" << message->body() << "\"" << std::endl;
}
void onQueryResponse(const TcpConnectionPtr&, const std::shared_ptr<basicQueryResponse>& message, muduo::Timestamp) {
    std::string body = message->body();
    if (!body.empty()) {
        std::cout << "[Pulled Message] " << body << std::endl;
    } else {
        std::cout << "[Pulled Message] (none available)" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 5555;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::atoi(argv[2]);

    muduo::net::InetAddress serverAddr(host, port);
    muduo::net::TcpClient client(&g_loop, serverAddr, "MQClient");
    g_dispatcher.registerMessageCallback<basicCommonResponse>(onCommonResponse);
    g_dispatcher.registerMessageCallback<basicConsumeResponse>(onConsumeResponse);
    g_dispatcher.registerMessageCallback<basicQueryResponse>(onQueryResponse);
    ProtobufCodec codec(std::bind(&ProtobufDispatcher::onProtobufMessage, &g_dispatcher,
                                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    g_codec = std::make_shared<ProtobufCodec>(codec);
    client.setMessageCallback(onMessage);
    client.connect();

    // Run the client loop in a separate thread
    std::thread loopThread([&]() { g_loop.loop(); });

    std::cout << "Connected to message queue server at " << host << ":" << port << std::endl;
    std::cout << "Commands:\n"
              << "open <cid>\n"
              << "close <cid>\n"
              << "exchange_declare <name> <direct|fanout|topic>\n"
              << "queue_declare <name>\n"
              << "bind <exch> <queue> <binding_key>\n"
              << "publish <exch> <routing_key> <message>\n"
              << "pull <cid>\n"
              << "consume <cid> <queue> <consumer_tag>\n"
              << "cancel <cid> <consumer_tag> <queue>\n"
              << "exit\n";

    std::string command;
    TcpConnectionPtr conn;
    // Wait for connection establishment
    while (!conn) {
        client.connection()->getLoop()->runAfter(0.1, [&](){}); // just yield CPU briefly
        conn = client.connection();
    }

    // Command loop
    while (true) {
        std::cout << "MQClient> ";
        if (!std::getline(std::cin, command)) break;
        if (command.empty()) continue;
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        if (cmd == "open") {
            std::string cid;
            iss >> cid;
            openChannelRequest req;
            req.set_rid("cli-open-" + cid);
            req.set_cid(cid);
            g_codec->send(conn, req);
        } else if (cmd == "close") {
            std::string cid;
            iss >> cid;
            closeChannelRequest req;
            req.set_rid("cli-close-" + cid);
            req.set_cid(cid);
            g_codec->send(conn, req);
        } else if (cmd == "exchange_declare") {
            std::string ename, etype;
            iss >> ename >> etype;
            ExchangeType exType = ExchangeType::DIRECT;
            if (etype == "fanout") exType = ExchangeType::FANOUT;
            else if (etype == "topic") exType = ExchangeType::TOPIC;
            declareExchangeRequest req;
            req.set_rid("cli-exdec-" + ename);
            req.set_cid("0");
            req.set_exchange_name(ename);
            req.set_exchange_type(exType);
            req.set_durable(false);
            req.set_auto_delete(false);
            g_codec->send(conn, req);
        } else if (cmd == "queue_declare") {
            std::string qname;
            iss >> qname;
            declareQueueRequest req;
            req.set_rid("cli-qdec-" + qname);
            req.set_cid("0");
            req.set_queue_name(qname);
            req.set_exclusive(false);
            req.set_durable(false);
            req.set_auto_delete(false);
            g_codec->send(conn, req);
        } else if (cmd == "bind") {
            std::string exch, qname, key;
            iss >> exch >> qname >> key;
            bindRequest req;
            req.set_rid("cli-bind-" + exch + "-" + qname);
            req.set_cid("0");
            req.set_exchange_name(exch);
            req.set_queue_name(qname);
            req.set_binding_key(key);
            g_codec->send(conn, req);
        } else if (cmd == "publish") {
            std::string exch, rkey, msg;
            iss >> exch >> rkey;
            std::getline(iss, msg);
            if (!msg.empty() && msg[0] == ' ') msg.erase(0, 1);
            basicPublishRequest req;
            req.set_rid("cli-pub-" + exch);
            req.set_cid("0");
            req.set_exchange_name(exch);
            req.set_body(msg);
            // set properties with routing key if provided
            if (!rkey.empty()) {
                BasicProperties* props = req.mutable_properties();
                props->set_routing_key(rkey);
                props->set_delivery_mode(DeliveryMode::UNDURABLE);
            }
            g_codec->send(conn, req);
        } else if (cmd == "pull") {
            std::string cid;
            iss >> cid;
            basicQueryRequest req;
            req.set_rid("cli-query-" + cid);
            req.set_cid(cid);
            g_codec->send(conn, req);
        } else if (cmd == "consume") {
            std::string cid, qname, tag;
            iss >> cid >> qname >> tag;
            basicConsumeRequest req;
            req.set_rid("cli-consume-" + cid);
            req.set_cid(cid);
            req.set_queue_name(qname);
            req.set_consumer_tag(tag);
            req.set_auto_ack(true);
            g_codec->send(conn, req);
        } else if (cmd == "cancel") {
            std::string cid, tag, qname;
            iss >> cid >> tag >> qname;
            basicCancelRequest req;
            req.set_rid("cli-cancel-" + cid);
            req.set_cid(cid);
            req.set_consumer_tag(tag);
            req.set_queue_name(qname);
            g_codec->send(conn, req);
        } else if (cmd == "exit") {
            break;
        } else {
            std::cout << "Unknown command." << std::endl;
        }
    }

    // Cleanup
    client.disconnect();
    g_loop.quit();
    loopThread.join();
    return 0;
}
