#ifndef _NET_ASIO_CONNECTION_H
#define _NET_ASIO_CONNECTION_H

#include "net.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include "message.h"
#include "message_dispatcher.h"

NAMESPACE_NETASIO_BEGIN

//////////////////////////////////////////////////////
//
// 消息缓冲区
//
//////////////////////////////////////////////////////
struct SharedBuffer
{
public:
	SharedBuffer(size_t size) : buff(new char[size]), size(size) {}

	boost::asio::mutable_buffers_1 asio_buff() const 
	{
		return boost::asio::buffer(buff.get(), size);
	}

public:
	boost::shared_array<char> buff;
	size_t size;
};

//////////////////////////////////////////////////////
//
// 一条TCP连接：
//  可以是客户端连接，也可以是服务器的客户端会话
//
//////////////////////////////////////////////////////
class Connection;
typedef boost::shared_ptr<Connection> ConnectionPtr;
typedef boost::function<void(ConnectionPtr, const char*, size_t)> MessageHandler;
typedef boost::function<void(ConnectionPtr, uint16, uint16)> StateChangeCallBack;
typedef boost::function<void(ConnectionPtr)> IOErrorCallBack;


class Connection :
	public boost::enable_shared_from_this<Connection>,
	private boost::noncopyable
{
public:
	Connection(boost::asio::io_service& io_service,
		boost::asio::io_service& strand_service,
		const MessageHandler& message_handler);

	virtual ~Connection();

	/// 连接开始，默认开始执行第一个异步读操作
	virtual void start();

	/// 停止所有异步操作并关闭连接
	virtual void stop();

	template <typename ProtoT>
	bool send(const ProtoT& proto)
	{
		if (isClosed()) return false;
		
		SharedBuffer sb(sizeof(MessageHeader) + proto.ByteSize());

		MessageHeader* mh = new (sb.buff.get()) MessageHeader;
		mh->type_first = ProtoT::TYPE1;
		mh->type_second = ProtoT::TYPE2;
		mh->size = proto.ByteSize();
		proto.SerializeToArray(sb.buff.get() + sizeof(MessageHeader), mh->size);

		std::cout << "SEND:" << mh->dumphex() << std::endl; 
		asyncWrite(sb);
		return true;
	}

public:
	boost::asio::ip::tcp::socket& socket() { return socket_; }
	boost::asio::io_service::strand& strand() { return strand_; };

	//
	// Getter And Setter
	//
	uint32 id() const { return id_; }
	uint32 type() const { return type_; }
	const std::string& name() const { return name_; }
	std::string debugString();

	void set_id(const uint32 id);
	void set_type(const uint32 type) { type_ = type; }
	void set_name(const std::string& name) { name_ = name; }
	

	//
	// 连接状态
	//
	enum ConnectionStateEnum
  	{
  		kState_Closed      = 0, // 关闭
  		kState_Resolving   = 1, // 正在进行地址解析
  		kState_Resolved    = 2, // 解析成功
  		kState_Connecting  = 3, // 正在连接/正在接收连接
  		kState_Connected   = 4, // 连接成功
  		kState_Verifying   = 5, // 正在验证连接(可选状态)
  		kState_Verified    = 6, // 连接验证通过(可选状态)
  		kState_Runing      = 7, // 进入随意收发状态 
  	};

  	void set_state(ConnectionStateEnum state);
  	uint16 state() { return state_; }
  	static const char* stateDescription(uint16 state);

  	bool isClosed() { return kState_Closed == state_; }
  	bool isConnected() { return kState_Connected == state_; }
  	bool isRunning() { return kState_Runing == state_; }

  	//
  	// 各种回调
  	//
  	void set_message_handler(const MessageHandler& handler) { message_handler_ = handler; }
  	void set_state_callback(const StateChangeCallBack& cb) { on_state_change_ = cb; }
  	void set_read_error_callback(const IOErrorCallBack& cb) { on_read_err_ = cb; }
  	void set_write_error_callback(const IOErrorCallBack& cb) { on_write_err_ = cb; }

protected:
	/// Handle completion of a read operation.
  	void handle_read(const boost::system::error_code& e);
  	void handle_write(const boost::system::error_code& err, SharedBuffer buff);

  	void asyncRead();
  	void asyncWrite(SharedBuffer buff);

  	bool readMessages();

  	virtual void onReadError(const boost::system::error_code& e);
  	virtual void onWriteError(const boost::system::error_code& e);

protected:
	/// 唯一标识
	uint32 id_;

	/// 连接类型
	uint32 type_;

	/// 当前状态
	volatile uint16 state_;

	/// 名称/描述
	std::string name_;

	/// Socket for the connection.
	boost::asio::ip::tcp::socket socket_;

	/// Strand to ensure the connection's handlers are not called concurrently.
	boost::asio::io_service::strand strand_;

	/// Buffer for incoming data.
	boost::asio::streambuf readbuf_;

	/// The handler used to process the incoming message.
	MessageHandler message_handler_;

	/// 连接状态改变回调
	StateChangeCallBack on_state_change_;

	/// 读/写操作错误回调
	IOErrorCallBack on_read_err_;
	IOErrorCallBack on_write_err_;
};

NAMESPACE_NETASIO_END

#endif // _NET_ASIO_CONNECTION_H