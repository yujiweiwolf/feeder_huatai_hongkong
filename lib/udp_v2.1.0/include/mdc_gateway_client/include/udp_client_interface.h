#pragma once
#include <string>
#include "base_define.h"
#include "MDSubscribe.pb.h"

NAMESPACE_BEGIN
class MessageHandle;
NAMESPACE_END

using namespace com::htsc::mdc::gateway;

namespace com {
namespace htsc {
namespace mdc {
namespace udp {
class UdpClientInterface
{
public:
	UdpClientInterface() {}
	virtual ~UdpClientInterface() {}

public:
	/**
	* 注册处理对象，必须在login之前设置
	* @param[in] handle 处理对象
	*/
	virtual void RegistHandle(MessageHandle* handle) = 0;

	/**
	* 使用账户密码登录配置服务器，获取组播地址
	* @param[in]  ip        配置服务器地址
	* @param[in]  port      配置服务器端口
	* @param[in]  user_name 用户名
	* @param[in]  password  密码
	* @param[in]  backups   备用服务器地址和端口
	* @param[out] int       登录结果
	*/
	virtual int LoginById(const std::string& ip, int port, const std::string& user_name, const std::string& password, const std::map<std::string, int>& backups) = 0;
	
	/**
	* 使用指定网卡地址来订阅所有取组播数据
	* @param[in]  interface_ip   用于订阅组播的网卡ip
	* @param[out] int            订阅结果
	*/
	virtual int SubscribeAll(const std::string& interface_ip) = 0;

	/**
	* 使用指定网卡地址来订阅指定类型数据
	* @param[in]  interface_ip   用于订阅组播的网卡ip
	* @param[in]  source_type    订阅数据类型
	* @param[out] int            订阅结果
	*/
	virtual int SubscribeBySourceType(const std::string& interface_ip, 
		::com::htsc::mdc::insight::model::SubscribeBySourceType* source_type) = 0;

	/**
	* 设置处理线程池线程数
	* @param[in] count 线程数
	*/
	virtual void SetWorkPoolThreadCount(short count) = 0;
};

}
}
}
}