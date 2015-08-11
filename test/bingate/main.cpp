/*
 * logic.cpp
 *
 *  Created on: 2015-1-17
 *      Author: qianqians
 */
#include "center.h"

#include <boost/make_shared.hpp>

#include <config/config.h>

namespace Fossilizid{
namespace center{
	
center::center(std::string filename, std::string key){
	isrun = true;

	boost::shared_ptr<config::config> _config = boost::make_shared<config::config>(filename);

	auto center_config = _config->get_value_dict("center");
	if (center_config == 0){
		throw std::exception("cannot find center config");
	} else{
		try{
			center_addr.first = center_config->get_value_string("ip");
			center_addr.second = (short)center_config->get_value_int("port");
		} catch(...){
			throw std::exception("center config field error");
		}
	}

	servernum = 0;

	_service = juggle::create_service();
	_process = boost::make_shared<juggle::process>();
	_channelservice = boost::make_shared<achieve::channelservice>();

	_svrsessioncontainer = boost::make_shared<achieve::sessioncontainer>(_channelservice, _process);

	auto center_config = _config->get_value_dict("key");
	if (center_config == 0){
		throw std::exception("cannot find this config");
	} else{
		auto set = boost::make_shared<std::vector<std::pair<std::string, short> > >();
		auto dict = _config->get_value_dict("blacklist");
		for (size_t i = 0; i < dict->get_list_size(); i++){
			auto e = dict->get_list_dict(i);
			auto ip = e->get_value_string("ip");
			auto port = (short)e->get_value_int("port");
			set->push_back(std::make_pair(ip, port));
		}
		_writeacceptor = boost::make_shared<acceptor::writeacceptor>(center_config->get_value_string("ip"), center_config->get_value_int("port"), set, _channelservice, _svrsessioncontainer);
	}
	_timerservice = timer::timerservice::createinstance();

	_module = boost::make_shared<module::center>(_process);
	_module->sigregister_db.connect(boost::bind(&center::register_db, this, _1, _2));
	_module->sigregister_gate.connect(boost::bind(&center::register_gate, this, _1, _2));
	_module->sigregister_routing.connect(boost::bind(&center::register_routing, this, _1, _2));
	_module->sigregister_logic.connect(boost::bind(&center::register_logic, this, _1, _2));
}

center::~center(){
}

void center::run(){
	_service->init();

	while (isrun.load()){
		auto btime = _service->unixtime();

		auto ptime = _timerservice->get_event_time() == 0 ? 0 : _timerservice->get_event_time() - btime;
		_channelservice->poll(ptime);

		_service->poll();

		_timerservice->poll(_service->unixtime());

		auto etime = timetmp = _service->unixtime();

		timetmp = etime - btime;
	}
}

void center::cancle_center(){
	isrun = false;
}

void center::svr_disconn(boost::shared_ptr<juggle::channel> ch){
	if (gatemap.find(ch) != gatemap.end()){
		gatemap.erase(ch);
		gateaddrmap.erase(ch);
	}
	if (logicmap.find(ch) != logicmap.end()){
		logicmap.erase(ch);
		logicaddrmap.erase(ch);
	}
	if (routingmap.find(ch) != routingmap.end()){
		routingmap.erase(ch);
		routingaddrmap.erase(ch);
	}
	if (dbmap.find(ch) != dbmap.end()){
		dbmap.erase(ch);
		dbaddrmap.erase(ch);
	}
}

int center::register_logic(boost::shared_ptr<Fossilizid::juggle::channel> ch, std::string ip, int port){
	logicaddrmap.insert(std::make_pair(ch, std::make_pair(ip, port)));
	logicmap.insert(std::make_pair(ch, boost::make_shared<sync::logic>(_process, ch)));

	return ++servernum;
}

int center::register_gate(boost::shared_ptr<Fossilizid::juggle::channel> ch, std::string ip, int port){
	gateaddrmap.insert(std::make_pair(ch, std::make_pair(ip, port)));
	gatemap.insert(std::make_pair(ch, boost::make_shared<sync::gate>(_process, ch)));

	return ++servernum;
}

int center::register_routing(boost::shared_ptr<Fossilizid::juggle::channel> ch, std::string ip, int port){
	routingaddrmap.insert(std::make_pair(ch, std::make_pair(ip, port)));
	routingmap.insert(std::make_pair(ch, boost::make_shared<sync::routing>(_process, ch)));

	return ++servernum;
}

int center::register_db(boost::shared_ptr<Fossilizid::juggle::channel> ch, std::string ip, int port){
	dbaddrmap.insert(std::make_pair(ch, std::make_pair(ip, port)));
	dbmap.insert(std::make_pair(ch, boost::make_shared<sync::dbproxy>(_process, ch)));

	return ++servernum;
}

} /* namespace routing */
} /* namespace Fossilizid */

