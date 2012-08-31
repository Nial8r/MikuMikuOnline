﻿//
// Session.cpp
//

#include "Command.hpp"
#include "Session.hpp"
#include "Utils.hpp"
#include "../Logger.hpp"
#include <boost/make_shared.hpp>
#include <string>

namespace network {

    Session::Session(boost::asio::io_service& io_service_tcp) :
      io_service_tcp_(io_service_tcp),
      socket_tcp_(io_service_tcp),
      encryption_(false),
      online_(true),
      login_(false),
      read_start_time_(time(nullptr)),
      write_start_time_(time(nullptr)),
      read_byte_sum_(0),
      write_byte_sum_(0),
      serialized_byte_sum_(0),
      compressed_byte_sum_(0),
      id_(0)
    {

    }

    Session::~Session()
    {
        Close();
    }

    void Session::Close()
    {
        socket_tcp_.close();
    }

    void Session::Send(const Command& command)
    {
        auto msg = Serialize(command);
        write_byte_sum_ += msg.size();
        UpdateWriteByteAverage();

        io_service_tcp_.post(boost::bind(&Session::DoWriteTCP, this, msg));
    }

    void Session::SyncSend(const Command& command)
    {
        auto msg = Serialize(command);
        write_byte_sum_ += msg.size();
        UpdateWriteByteAverage();

        try {
            boost::asio::write(
                    socket_tcp_, boost::asio::buffer(msg.data(), msg.size()),
                boost::asio::transfer_all());
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

    double Session::GetReadByteAverage() const
    {
        return 1.0f * read_byte_sum_ / (time(nullptr) - read_start_time_);
    }

    double Session::GetWriteByteAverage() const
    {
        return 1.0f * write_byte_sum_ / (time(nullptr) - write_start_time_);
    }

    void Session::UpdateReadByteAverage()
    {
        unsigned long elapsed_time = time(nullptr) - read_start_time_;
        if (elapsed_time >= BYTE_AVERAGE_REFRESH_SECONDS) {
            read_byte_sum_ /= 2;
            read_start_time_ = time(nullptr) - elapsed_time / 2;
        }
    }

    void Session::UpdateWriteByteAverage()
    {
        unsigned long elapsed_time = time(nullptr) - write_start_time_;
        if (elapsed_time >= BYTE_AVERAGE_REFRESH_SECONDS) {
            write_byte_sum_ /= 2;
            write_start_time_ = time(nullptr) - elapsed_time / 2;
        }
    }

    void Session::EnableEncryption()
    {
        encryption_ = true;
    }

    Encrypter& Session::encrypter()
    {
        return encrypter_;
    }

    tcp::socket& Session::tcp_socket()
    {
        return socket_tcp_;
    }

    UserID Session::id() const
    {
        return id_;
    }

    void Session::set_id(UserID id)
    {
        id_ = id;
    }

    bool Session::online() const
    {
        return online_;
    }

    std::string Session::global_ip() const
    {
        return global_ip_;
    }

    uint16_t Session::udp_port() const{
        return udp_port_;
    }

    void Session::set_global_ip(const std::string& global_ip)
    {
        global_ip_ = global_ip;
    }

    void Session::set_udp_port(uint16_t udp_port)
    {
        udp_port_ = udp_port;
    }

    int Session::serialized_byte_sum() const
    {
        return serialized_byte_sum_;
    }

    int Session::compressed_byte_sum() const
    {
        return compressed_byte_sum_;
    }

    bool Session::operator==(const Session& s)
    {
        return id_ == s.id_;
    }

    bool Session::operator!=(const Session& s)
    {
        return !operator==(s);
    }

    std::string Session::Serialize(const Command& command)
    {
        unsigned int header = command.header();
        std::string body = command.body();

        serialized_byte_sum_ += body.size();

        // 圧縮
        if (!static_cast<bool>(header & COMPRESSED_FLAG) &&
                body.size() >= COMPRESS_MIN_LENGTH) {
            body = Utils::SnappyCompress(body);
            header |= COMPRESSED_FLAG;
        }

        compressed_byte_sum_ += body.size();

        std::string msg(reinterpret_cast<char*>(&header), sizeof(int));
        msg += body;

        // 暗号化
        if (encryption_) {
            msg = encrypter_.Encrypt(msg);
        }

        return Utils::Encode(msg);
    }

    Command Session::Deserialize(const std::string& msg)
    {
        std::string decoded_msg = Utils::Decode(msg);

        // 復号化
        if (encryption_) {
            decoded_msg = encrypter_.Decrypt(decoded_msg);
        }

        header::CommandHeader header = *reinterpret_cast<const header::CommandHeader*>(decoded_msg.data());
        std::string body = decoded_msg.substr(sizeof(uint32_t));

        // 伸長
        if (static_cast<bool>(header & COMPRESSED_FLAG)) {
            body = Utils::SnappyUncompress(body);
            header = static_cast<header::CommandHeader>(header & ~(COMPRESSED_FLAG));
        }

        return Command(header, body, shared_from_this());
    }

    void Session::ReceiveTCP(const boost::system::error_code& error)
    {
        if (!error) {
            std::string buffer(boost::asio::buffer_cast<const char*>(receive_buf_.data()),receive_buf_.size());
            auto length = buffer.find_last_of(NETWORK_UTILS_DELIMITOR);

            if (length != std::string::npos) {

                receive_buf_.consume(length+1);
                buffer.erase(length+1);

                while (!buffer.empty()) {
                    std::string msg;

                    while (!buffer.empty() && buffer[0]!=NETWORK_UTILS_DELIMITOR)
                    {
                        msg += buffer[0];
                        buffer.erase(0,1);
                    }
                    buffer.erase(0,1);

                    read_byte_sum_ += msg.size();
                    UpdateReadByteAverage();

                    FetchTCP(msg);
                }

                boost::asio::async_read_until(socket_tcp_,
                    receive_buf_, NETWORK_UTILS_DELIMITOR,
                    boost::bind(
                      &Session::ReceiveTCP, shared_from_this(),
                      boost::asio::placeholders::error));

            }

        } else {
            FatalError();
        }
    }

    void Session::DoWriteTCP(const std::string msg)
    {
        bool write_in_progress = !send_queue_.empty();
        send_queue_.push(msg);
        if (!write_in_progress && !send_queue_.empty())
        {
           
          boost::shared_ptr<std::string> s = 
              boost::make_shared<std::string>(msg.data(), msg.size());

          boost::asio::async_write(socket_tcp_,
              boost::asio::buffer(s->data(), s->size()),
              boost::bind(&Session::WriteTCP, this,
                boost::asio::placeholders::error, s));
        }
    }

    void Session::WriteTCP(const boost::system::error_code& error, boost::shared_ptr<std::string> holder)
    {
        if (!error) {
            if (!send_queue_.empty()) {
                  send_queue_.pop();
                  if (!send_queue_.empty())
                  {

                    boost::shared_ptr<std::string> s = 
                        boost::make_shared<std::string>(send_queue_.front().data(), send_queue_.front().size());

                    boost::asio::async_write(socket_tcp_,
                        boost::asio::buffer(s->data(), s->size()),
                        boost::bind(&Session::WriteTCP, this,
                          boost::asio::placeholders::error, s));
                  }
            }
        } else {
            FatalError();
        }
    }

    void Session::FetchTCP(const std::string& msg)
    {
        if (msg.size() >= sizeof(int)) {
            if (on_receive_) {
                (*on_receive_)(Deserialize(msg));
            }
        }
    }

    void Session::FatalError()
    {
        if (online_) {
            online_ = false;
            if (on_receive_) {
                if (id_ > 0) {
                    (*on_receive_)(FatalConnectionError(id_));
                } else {
                    (*on_receive_)(FatalConnectionError());
                }
            }
        }
    }

    void Session::set_on_receive(CallbackFuncPtr func)
    {
        on_receive_ = func;
    }


}