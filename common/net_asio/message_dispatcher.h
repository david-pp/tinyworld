#ifndef _NET_ASIO_MESSAGE_DISPATCHER_H
#define _NET_ASIO_MESSAGE_DISPATCHER_H

#include <map>
#include <iostream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>
#include "message.h"

///////////////////////////////////////////////////////////
//
//  基于Protobuf的消息处理和分发(2015/12/18 by David)
//   
//   Protobuf消息的约束：
//      1> MSG::TYPE1  - 主消息号
//      2> MSG::TYPE2  - 次消息号
//
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
//
// 消息处理Handler
//
///////////////////////////////////////////////////////////

//
// 消息处理基类
//
class ProtobufMsgHandler : boost::noncopyable
{
public:
	struct Args
	{
	};

	template <typename T1>
	struct ArgsT_1 : public Args
	{
		ArgsT_1(const T1& a1) : arg1(a1) {}
		T1 arg1;
	};

	template <typename T1, typename T2>
	struct ArgsT_2 : public Args
	{
		ArgsT_2(const T1& a1, const T2& a2) : arg1(a1), arg2(a2) {}
		T1 arg1;
		T2 arg2;
	};

	typedef google::protobuf::Message Message;
	typedef boost::shared_ptr<Message> MessagePtr;
	typedef boost::shared_ptr<Args> ArgsPtr;

public:
	ProtobufMsgHandler(uint16 msgtype)
		: msgtype_(msgtype) {}

	virtual ~ProtobufMsgHandler() {}
	virtual void process(MessagePtr msg, ArgsPtr args) const = 0;

	// prototype
	virtual Message* newMessage() { return NULL; }

	uint16 msgtype() { return msgtype_; }
	uint16 type1() { return MSG_TYPE_1(msgtype_); }
	uint16 type2() { return MSG_TYPE_2(msgtype_); }

private:
	uint16 msgtype_;
};

typedef boost::shared_ptr<ProtobufMsgHandler> ProtobufMsgHandlerPtr;


//
// 消息处理形如：void handler(void)
//
template <typename MSG>
class ProtobufMsgHandlerT : public ProtobufMsgHandler
{
public:
	typedef boost::function<void(void)> Handler;

	ProtobufMsgHandlerT(uint16 msgtype, const Handler& handler) 
		: ProtobufMsgHandler(msgtype), handler_(handler) {}

	virtual Message* newMessage() { return new MSG; }

	virtual void process(MessagePtr msg, ArgsPtr args) const
	{
		handler_();
	}

private:
	Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message)
//
template <typename MSG>
class ProtobufMsgHandlerT_0 : public ProtobufMsgHandler
{
public:
	typedef boost::function<void(MSG*)> Handler;

	ProtobufMsgHandlerT_0(uint16 msgtype, const Handler& handler) 
		: ProtobufMsgHandler(msgtype), handler_(handler) {}

	virtual Message* newMessage() { return new MSG; }

	virtual void process(MessagePtr msg, ArgsPtr args) const
	{
		handler_((MSG*)msg.get());
	}

private:
	Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message, T1 arg1)
//
template <typename MSG, typename T1>
class ProtobufMsgHandlerT_1 : public ProtobufMsgHandler
{
public:
	typedef boost::function<void(MSG*, T1)> Handler;

	typedef ProtobufMsgHandler::ArgsT_1<T1> ArgsType;

	ProtobufMsgHandlerT_1(uint16 msgtype, const Handler& handler) 
		: ProtobufMsgHandler(msgtype), handler_(handler) {}

	virtual Message* newMessage() { return new MSG; }

	virtual void process(MessagePtr msg, ArgsPtr args) const
	{
		if (args)
		{
			ArgsType* a = (ArgsType*)args.get();
			handler_((MSG*)msg.get(),  a->arg1);
		}
		else
			handler_((MSG*)msg.get(), T1());
	}

private:
	Handler handler_;
};

//
// 消息处理形如：void handler(MSG* message, T1 arg1, T2 arg2)
//
template <typename MSG, typename T1, typename T2>
class ProtobufMsgHandlerT_2 : public ProtobufMsgHandler
{
public:
	typedef boost::function<void(MSG*, T1, T2)> Handler;

	typedef ProtobufMsgHandler::ArgsT_2<T1, T2> ArgsType;

	ProtobufMsgHandlerT_2(uint16 msgtype, const Handler& handler) 
		: ProtobufMsgHandler(msgtype), handler_(handler) {}

	virtual Message* newMessage() { return new MSG; }

	virtual void process(MessagePtr msg, ArgsPtr args) const
	{
		if (args)
		{
			ArgsType* a = (ArgsType*)args.get();
			handler_((MSG*)msg.get(),  a->arg1, a->arg2);
		}
		else
			handler_((MSG*)msg.get(), T1(), T2());
	}

private:
	Handler handler_;
};

///////////////////////////////////////////////////////////
//
// 消息分发器
//
///////////////////////////////////////////////////////////
class ProtobufMsgDispatcher
{
public:
	//
	// 绑定消息处理函数，可以是普通的函数、函数对象、成员函数等可执行体
	//
	//  - bind0<MSG>(handler)           handler没有任何参数
	//  - bind<MSG>(handler)            handler没有额参数
	//  - bind<MSG, T1>(handler)        handler有1个额外参数
	//  - bind<MSG, T1, T2>(handler)    handler有2个额外参数
	// 
	template <typename MSG>
	bool bind0(const typename ProtobufMsgHandlerT<MSG>::Handler& handler)
	{
		ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT<MSG>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), handler));
		if (!bindHandlerPtr(h))
		{
			std::cerr << "[FATAL] " << __PRETTY_FUNCTION__ 
			          << ": (" << h->type1() 
			          << "," << h->type2() 
			          << ")" << std::endl;
			return false;
		}
		return true;
	}

	template <typename MSG>
	bool bind(const typename ProtobufMsgHandlerT_0<MSG>::Handler& handler)
	{
		ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_0<MSG>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), handler));
		if (!bindHandlerPtr(h))
		{
			std::cerr << "[FATAL] " << __PRETTY_FUNCTION__ 
			          << ": (" << h->type1() 
			          << "," << h->type2() 
			          << ")" << std::endl;
			return false;
		}
		return true;
	}

	template <typename MSG, typename T1>
	bool bind(const typename ProtobufMsgHandlerT_1<MSG, T1>::Handler& handler)
	{
		ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_1<MSG, T1>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), handler));
		if (!bindHandlerPtr(h))
		{
			std::cerr << "[FATAL] " << __PRETTY_FUNCTION__ 
			          << ": (" << h->type1() 
			          << "," << h->type2() 
			          << ")" << std::endl;
			return false;
		}
		return true;
	}

	template <typename MSG, typename T1, typename T2>
	bool bind(const typename ProtobufMsgHandlerT_2<MSG, T1, T2>::Handler& handler)
	{
		ProtobufMsgHandlerPtr h(new ProtobufMsgHandlerT_2<MSG, T1, T2>(MAKE_MSG_TYPE(MSG::TYPE1, MSG::TYPE2), handler));
		if (!bindHandlerPtr(h))
		{
			std::cerr << "[FATAL] " << __PRETTY_FUNCTION__ 
			          << ": (" << h->type1() 
			          << "," << h->type2() 
			          << ")" << std::endl;
			return false;
		}
		return true;
	}


	bool bindHandlerPtr(ProtobufMsgHandlerPtr handler)
	{
		if (!handler) return false;

		if (handlers_.find(handler->msgtype()) != handlers_.end())
			return false;

		handlers_.insert(std::make_pair(handler->msgtype(), handler));
		return true;
	}

	//
	// 消息分发
	//
	bool dispatch(uint16 msgtype, void* msg, uint32 msgsize)
	{
		return dispatch_(msgtype, msg, msgsize, ProtobufMsgHandler::ArgsPtr());
	}

	template <typename T1>
	bool dispatch(uint16 msgtype, void* msg, uint32 msgsize, T1 arg1)
	{
		typedef typename ProtobufMsgHandler::ArgsT_1<T1> ArgsType;
		ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1));
		return dispatch_(msgtype, msg, msgsize, args);
	}

	template <typename T1, typename T2>
	bool dispatch(uint16 msgtype, void* msg, uint32 msgsize, T1 arg1, T2 arg2)
	{
		typedef typename ProtobufMsgHandler::ArgsT_2<T1, T2> ArgsType;
		ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1, arg2));
		return dispatch_(msgtype, msg, msgsize, args);
	}

	bool dispatchMsg(uint16 msgtype, ProtobufMsgHandler::MessagePtr msg)
	{
		return dispatch_(msgtype, msg, ProtobufMsgHandler::ArgsPtr());
	}

	template <typename T1>
	bool dispatchMsg1(uint16 msgtype, ProtobufMsgHandler::MessagePtr msg, T1 arg1)
	{
		typedef typename ProtobufMsgHandler::ArgsT_1<T1> ArgsType;
		ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1));
		return dispatch_(msgtype, msg, args);
	}

	template <typename T1, typename T2>
	bool dispatchMsg2(uint16 msgtype, ProtobufMsgHandler::MessagePtr msg, T1 arg1, T2 arg2)
	{
		typedef typename ProtobufMsgHandler::ArgsT_2<T1, T2> ArgsType;
		ProtobufMsgHandler::ArgsPtr args(new ArgsType(arg1, arg2));
		return dispatch_(msgtype, msg, args);
	}

	//
	// 消息构造函数
	//
	ProtobufMsgHandler::MessagePtr makeMessage(uint16 msgtype, void* msg, uint32 msgsize)
	{
		ProtobufMsgHandlerMap::iterator it = handlers_.find(msgtype);
		if (it != handlers_.end())
		{
			ProtobufMsgHandler::MessagePtr proto(it->second->newMessage());
			if (proto && proto->ParseFromArray(msg, msgsize))
			{
				return proto;
			}
		}

		return ProtobufMsgHandler::MessagePtr();
	}

protected:
	bool dispatch_(uint16 msgtype, void* msg, uint32 msgsize, ProtobufMsgHandler::ArgsPtr args)
	{
		ProtobufMsgHandlerMap::iterator it = handlers_.find(msgtype);
		if (it != handlers_.end())
		{
			ProtobufMsgHandler::MessagePtr proto(it->second->newMessage());
			if (proto && proto->ParseFromArray(msg, msgsize))
			{
				it->second->process(proto, args);
				return true;
			}
		}

		return false;
	}

	bool dispatch_(uint16 msgtype, ProtobufMsgHandler::MessagePtr msg, ProtobufMsgHandler::ArgsPtr args)
	{
		ProtobufMsgHandlerMap::iterator it = handlers_.find(msgtype);
		if (it != handlers_.end())
		{
			it->second->process(msg, args);
		}
		return false;
	}

private:
	typedef std::map<uint16, ProtobufMsgHandlerPtr> ProtobufMsgHandlerMap;

	ProtobufMsgHandlerMap handlers_;
};


#endif // _NET_ASIO_MESSAGE_DISPATCHER_H
