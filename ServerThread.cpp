#include <iostream>
#include <memory>
#include <condition_variable>
#include <mutex>

#include "ServerThread.h"
#include "ServerStub.h"
#include "ServerSocket.h"


CustomerRecord LaptopFactory::
GetCustomerRecord(Request rqst) {
	CustomerRecord crcd;

	map_lock.lock();
	int inMap = customer_map.count(rqst.GetCustomerId()) > 0;
	int last_order = -1;
	int customer_id = -1;

	if (inMap){
		last_order = customer_map[rqst.GetCustomerId()];
		customer_id = rqst.GetCustomerId();
	}

	map_cv.notify_all();
	map_lock.unlock();

	crcd.SetRecord(customer_id, last_order);
	return crcd;
}

LaptopInfo LaptopFactory::
CreateLaptop(Request rqst, int engineer_id) {
	LaptopInfo laptop;
	laptop.CopyOrder(rqst);
	laptop.SetEngineerId(engineer_id);

	std::promise<LaptopInfo> prom;
	std::future<LaptopInfo> fut = prom.get_future();

	std::unique_ptr<AdminRequest> rqstpt = 
		std::unique_ptr<AdminRequest>(new AdminRequest);
	rqstpt->laptop = laptop;
	rqstpt->prom = std::move(prom);

	erq_lock.lock();
	erq.push(std::move(rqstpt));
	erq_cv.notify_one();
	erq_lock.unlock();

	laptop = fut.get();
	return laptop;
}

void LaptopFactory::
EngineerThread(std::unique_ptr<ServerSocket> socket, int id) {
	int engineer_id = id;
	int request_type;
	Request rqst;
	LaptopInfo laptop;
	CustomerRecord crcd;
	int ident;
	ReplicationRequest rprqst;

	ServerStub stub;

	stub.Init(std::move(socket));
	
	while (true) {
		ident = stub.ReceiveIdent();
		if(ident == 1){
			rqst = stub.ReceiveRequest();

			if (!rqst.IsValid()) {
				break;
			}
			request_type = rqst.GetRequestType();
			
			switch (request_type) {
				case 1:
					laptop = CreateLaptop(rqst, engineer_id);
					stub.ShipLaptop(laptop);
					break;
				case 2:
					crcd = GetCustomerRecord(rqst);
					stub.ReturnRecord(crcd);
					break;
				default:
					break;
					//std::cout << "Undefined request type in server: "
					//	<< request_type << std::endl;
			}
		} else if(ident == 2) {
			rprqst = stub.ReceiveRepReq();

			if(!rprqst.IsValid()) {
				primary_id = -1;
				break;
			}
			primary_id = rprqst.GetFactoryId();

			struct MapOp new_log = {rprqst.GetUpdateMap().opcode, rprqst.GetUpdateMap().arg1, rprqst.GetUpdateMap().arg2};

			smr_lock.lock();
			smr_log.push_back(new_log);
			smr_cv.notify_all();
			last_index = smr_log.size() - 1;
			smr_lock.unlock();

			map_lock.lock();

			if(customer_map.size() < 1){
				customer_map.insert({smr_log[rprqst.GetCommittedIndex()].arg1, smr_log[rprqst.GetCommittedIndex()].arg2});
			} else {
				customer_map[smr_log[rprqst.GetCommittedIndex()].arg1] = smr_log[rprqst.GetCommittedIndex()].arg2;
			}

			map_cv.notify_all();
			map_lock.unlock();

			stub.SendRepResponse();
		} else {
			primary_id = -1;
			break;
		}
	}
		
}

void LaptopFactory::ProductionAdminThread(int id, int uid) {
	ServerStub stub;
	
	stub.peer_sockets.resize(peer_vector.size());
	factory_id = uid;

	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	while (true) {
		ul.lock();

		if (erq.empty()) {
			erq_cv.wait(ul, [this]{ return !erq.empty(); });
		}

		auto rqstpt = std::move(erq.front());
		erq.pop();

		int i = 0;
		for(PeerServer peer: peer_vector) {
			if(stub.Connect(peer, i) == 0){
				peer_vector[i].is_up = 0;
			};
			i++;
		}

		ul.unlock();
		//std::this_thread::sleep_for(std::chrono::microseconds(100));
		struct MapOp new_log = {1, rqstpt->laptop.GetCustomerId(), rqstpt->laptop.GetOrderNumber()};

		smr_lock.lock();
		smr_log.push_back(new_log);
		smr_cv.notify_all();
		last_index = smr_log.size() - 1;
		smr_lock.unlock();

		if(primary_id != factory_id){
			customer_map[smr_log[smr_log.size() -2].arg1] = smr_log[smr_log.size() - 2].arg2;
			committed_index = smr_log.size() - 2;
		}

		ReplicationRequest rprqst;
		rprqst.SetRequest(factory_id, last_index, committed_index, new_log);
		int n = 0;
		for(PeerServer peer: peer_vector) {
			if(peer.is_up == 1) {
				if(!stub.SendRepReq(rprqst, peer, n)){
					peer.is_up = 0;
				} else {	
					stub.ReceiveIdleResp(peer, n);
				}
			}
			n++;
		}
		
		map_lock.lock();
		customer_map[rqstpt->laptop.GetCustomerId()] = rqstpt->laptop.GetOrderNumber();
		map_cv.notify_all();
		map_lock.unlock();

		rqstpt->laptop.SetAdminId(id);
		rqstpt->prom.set_value(rqstpt->laptop);	
		
		committed_index = last_index;

		primary_id = factory_id;
	}
}

void LaptopFactory::AddPeer(PeerServer new_peer) {
	peer_vector.push_back(new_peer);
}
