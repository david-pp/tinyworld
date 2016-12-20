#ifndef _LOGIN_ACCOUNT_H
#define _LOGIN_ACCOUNT_H

#include "protos/account.pb.h"
#include "loginserver.h"
#include <boost/lexical_cast.hpp>

void onRegisterWithDeviceID(Cmd::Account::RegisterWithDeviceIDRequest* proto, 
	NetAsio::ConnectionPtr conn)
{
	ScopedMySqlConnection mysql(LoginServer::account_dbpool);
	if (mysql)
	{
		Cmd::Account::RegisterWithDeviceIDReply reply;

		try {
			mysqlpp::Query query = mysql->query();
			query << "SELECT * FROM account WHERE `device_id`='"<< proto->device_id() << "'";
			mysqlpp::StoreQueryResult res = query.store();
			if (res && res.num_rows() == 1)
			{
				reply.set_retcode(1);
				reply.set_accid(boost::lexical_cast<uint32>(res[0]["accid"]));
				reply.set_username(res[0]["username"]);
				reply.set_passwd(res[0]["passwd"]);
				conn->send(reply);

				LOG4CXX_WARN(logger, "[" << boost::this_thread::get_id() << "] " 
				 	<< "设备ID重复"
				 	<< proto->GetTypeName() << ":\n" 
				 	<< proto->DebugString());

				return;
			}

			query.reset();
			query << "INSERT INTO account(device_type, device_id) VALUES('" 
				  << proto->device_type()  << "', '"
				  << proto->device_id() << "')";

			mysqlpp::SimpleResult sres = query.execute();

			reply.set_retcode(0);
			reply.set_accid(sres.insert_id());
			reply.set_username(std::string("#tmpname#") + boost::lexical_cast<std::string>(sres.insert_id()));
			reply.set_passwd(std::string("#tmppasswd#") + boost::lexical_cast<std::string>(sres.insert_id()));
			conn->send(reply);

			query.reset();
			query << "UPDATE account SET username='" 
			      << reply.username() << "', passwd='" 
			      << reply.passwd() << "' WHERE accid=" << reply.accid(); 
			query.execute();

			LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " 
				 	<< "注册设备成功\n" 
				 	<< reply.DebugString());
		}
		catch (std::exception& err)
		{
			LOG4CXX_WARN(logger, "onRegisterWithDeviceID error:" << err.what());
		}
	}
}

void onRegisterWithNamePassword(Cmd::Account::RegisterWithNamePasswordRequest* proto, 
	NetAsio::ConnectionPtr conn)
{
	ScopedMySqlConnection mysql(LoginServer::account_dbpool);
	if (mysql)
	{
	}
}


void onRegisterBindNamePassword(Cmd::Account::RegisterBindNamePasswordRequest* proto,
	NetAsio::ConnectionPtr conn)
{
	ScopedMySqlConnection mysql(LoginServer::account_dbpool);
	if (mysql)
	{
	}
}

void onLoginWithNamePasswd(Cmd::Account::LoginWithNamePasswdReq* proto, 
	NetAsio::ConnectionPtr conn)
{
	ScopedMySqlConnection mysql(LoginServer::account_dbpool);
	if (mysql)
	{
		try {

			Cmd::Account::LoginWithNamePasswdRep reply;

			mysqlpp::Query query = mysql->query();
			query << "SELECT * FROM account WHERE `username`='"
			      << proto->username() << "' AND `passwd`='"
			      << proto->passwd() << "'";

			mysqlpp::StoreQueryResult res = query.store();
			if (!res || res.num_rows() == 0)
			{
				reply.set_retcode(1);
				conn->send(reply);

				LOG4CXX_WARN(logger, "[" << boost::this_thread::get_id() << "] " 
				 	<< "用户名或密码错误"
				 	<< proto->GetTypeName() << ":\n" 
				 	<< proto->DebugString());
				return;
			}

			std::pair<std::string, uint32> address = LoginServer::instance()->getGateAddress();

			reply.set_retcode(0);
			reply.set_accid(boost::lexical_cast<uint32>(res[0]["accid"]));
			reply.set_gate_ip(address.first);
			reply.set_gate_port(address.second);
			reply.set_gate_token("123456");
			conn->send(reply);


			LOG4CXX_INFO(logger, "[" << boost::this_thread::get_id() << "] " 
				 	<< "用户名密码验证通过\n" 
				 	<< reply.DebugString());
		}
		catch (std::exception& err)
		{
			LOG4CXX_WARN(logger, "onLoginWithNamePasswd error:" << err.what());
		}
	}
}


#endif // _LOGIN_ACCOUNT_H