#include <iostream>
#include <memory>

#include "ServerThread.h"
#include "ServerStub.h"

std::chrono::duration<double, std::micro> timeout = std::chrono::microseconds(4000000);


CustomerRecord LaptopFactory::AccessCustomerMap(CustomerRequest order) {
	CustomerRecord record;
	int id = order.GetCustomerId();
	map_lock.lock();
	if(customer_map.count(id)){
		record.SetRecord(id,customer_map[id]);
	}
	else{
		record.SetRecord(-1,-1);
	}
	// map_cv.notify_one();
	map_lock.unlock();

	return record;
}

LaptopInfo LaptopFactory::
CreateLaptop(CustomerRequest order, int engineer_id) {
	LaptopInfo laptop;
	laptop.CopyOrder(order);
	laptop.SetEngineerId(engineer_id);

	std::promise<LaptopInfo> prom;
	std::future<LaptopInfo> fut = prom.get_future();

	std::unique_ptr<ExpertRequest> req = std::unique_ptr<ExpertRequest>(new ExpertRequest);
	req->laptop = laptop;
	req->prom = std::move(prom);

	erq_lock.lock();
	erq.push(std::move(req));
	erq_cv.notify_one();
	erq_lock.unlock();

	laptop = fut.get();
	return laptop;
}

void LaptopFactory::
EngineerThread(std::unique_ptr<ServerSocket> socket, int id) {
	int engineer_id = id;
	int request_type;
	CustomerRequest order;
	int IFA_id;
	LaptopInfo laptop;
	CustomerRecord record;
	ServerStub stub;
	ReplicationRecord rep_record;
	MapOp smr_obj;
	ConfigUpdate update;

	stub.Init(std::move(socket));
	IFA_id = stub.ReceiveId();

	// printf("Primary id %d\n", IFA_id);

	while (true) {

		if(IFA_id>=0){
					// printf("Inside Replication Function\n");
					rep_record = stub.ReceiveReplicationRequest();


					if(rep_record.GetFactoryId() == -1){
						peer_isalive[IFA_id] = false;
						primary_id = -1;
						printf("Leader went down, Closing Follower thread \n");
						break;
					}

					//HeartBeat check
					else if(rep_record.GetFactoryId() == -10){
						printf("Recieved HeartBeat\n");
						start_time = std::chrono::high_resolution_clock::now();
						stub.SendReplicationAck();
						continue;
					}

					map_lock.lock();

					primary_id = rep_record.GetFactoryId();
					last_index = rep_record.GetLastIndex();
					rep_record.GetMapObj(smr_obj);
					smr_log.push_back(smr_obj);


					if(rep_record.GetCommitedIndex()>=0){
						committed_index = rep_record.GetCommitedIndex();
						if(customer_map.count(smr_log[committed_index].arg1)){
							customer_map[smr_log[committed_index].arg1] = smr_log[committed_index].arg2;
						}

						else{
							customer_map.insert(std::pair<int, int>(smr_log[committed_index].arg1, smr_log[committed_index].arg2));
						}

					}
					// map_cv.notify_one();
					map_lock.unlock();

					stub.SendReplicationAck();

		}
		else if(IFA_id == -1){
			order = stub.ReceiveRequest();
			if (!order.IsValid()) {
				break;
			}
			request_type = order.GetRequestType();
			switch (request_type) { // harsh: build request_type functions.
				case 1:
					laptop = CreateLaptop(order, engineer_id);
					stub.SendLaptop(laptop);
					break;
				case 2:
					record = AccessCustomerMap(order);
					stub.ReturnRecord(record);
					break;
				default:
					std::cout << "Undefined request type: "<< request_type << std::endl;

			}
		}
		else {
			update = stub.ReceiveUpdateRequest();

			if(update.IsValid()) {	
				update.Print();
			}

			//create locks on peer vectors
			// if(update.type == 1) {
			// 	peer_ips.insert(std::pair<int, std::string>(update.id, update.ip));
			// 	peer_ports.insert(std::pair<int,int>(update.id, update.port));
			// 	peer_isalive.insert(std::pair<int,bool>(update.id, true));
			// } else {
			// 	peer_ips.erase(peer_ips.find(update.id));
			// 	peer_ports.erase(peer_ports.find(update.id));
			//  peer_isalive.erase(peer_isalive.find(update.id));
			// }
		}
	}

	// printf("Exiting while loop\n");
}

void LaptopFactory::AdminThread(int id) {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	MapOp smr_obj;
	ServerStub stub;
	bool connection_flag = false;
	// stub.peer_sockets.resize(n_peers);
	ReplicationRecord rep_record;


	while (true) {
		ul.lock();

		if (erq.empty()) {
			erq_cv.wait(ul, [this]{ return !erq.empty(); });
		}

		auto req = std::move(erq.front());
		erq.pop();

		if(primary_id != unique_id){
			primary_id = unique_id;
		}
		if(!connection_flag){

			for (auto const& peer_info : peer_ips)
			{
				int peer_id = peer_info.first;
				std::string peer_ip = peer_info.second;
				int port = peer_ports[peer_id];
				if(stub.peer_sockets.count(peer_id) == 0){
					ServerSocket temp_socket;
					stub.peer_sockets.insert(std::pair<int,ServerSocket>(peer_id, temp_socket));
				}
				//Add peer alive code here
				if(peer_isalive[peer_id]){
					if (!stub.peer_sockets[peer_id].Init(peer_ip, port)) {
						std::cout << "Server id " << peer_id << " failed to connect, Backup Server down: "<< peer_id << std::endl;
						peer_isalive[peer_id]= false;
					}
					else{
						stub.SendAdminId(&stub.peer_sockets[peer_id], unique_id);
					}

				}

			}
			connection_flag = true;
		}

		ul.unlock();

		log_lock.lock();

		smr_obj.opcode = 1;
		smr_obj.arg1 = req->laptop.GetCustomerId();
		smr_obj.arg2 = req->laptop.GetOrderNumber();

		smr_log.push_back(smr_obj);
		last_index++;
		rep_record.SetRecord(unique_id, committed_index, last_index, smr_obj);
		// rep_record.Print();
		// int total_acks = 0;

		for (auto const& peer_info : peer_isalive){
			int peer_id = peer_info.first;
			bool peer_alive = peer_info.second;
			// printf("peer_id %d \n", peer_id);
			if(peer_alive){
				if(!stub.SendReplicationRequest(&stub.peer_sockets[peer_id], rep_record)){
					printf("Backup Server Down: %d\n", peer_id);
					peer_isalive[peer_id] = false;

				 }
				 else{
					 // printf("peer_id_alive %d\n",peer_id);
					 stub.ReceiveReplicationAck(&stub.peer_sockets[peer_id]);
	 				 // printf("Received Ack from peer %d\n",peer_id);
				 }

			}
		}

		log_lock.unlock();

		map_lock.lock();

			if(customer_map.count(smr_log[last_index].arg1)){
				customer_map[smr_log[last_index].arg1] = smr_log[last_index].arg2;
			}

			else{
				customer_map.insert(std::pair<int, int>(smr_log[last_index].arg1, smr_log[last_index].arg2));
			}

			committed_index = last_index;


		map_lock.unlock();

		// std::this_thread::sleep_for(std::chrono::microseconds(100));
		req->laptop.SetAdminId(id);
		req->prom.set_value(req->laptop);
	}
}


void LaptopFactory::HeartbeatThread() {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	MapOp smr_obj;
	bool connection_flag = false;
	ServerStub stub;
	ReplicationRecord rep_record;

	std::chrono::time_point<std::chrono::high_resolution_clock> curr;
	std::chrono::duration<double> elapsed_time, adjusted_timeout;
	adjusted_timeout = timeout - std::chrono::microseconds(5000);

	while (true) {

		curr = std::chrono::high_resolution_clock::now();
		elapsed_time = curr - start_time;

		ul.lock();
		if(!connection_flag){

			for (auto const& peer_info : peer_ips)
			{
				int peer_id = peer_info.first;
				std::string peer_ip = peer_info.second;
				int port = peer_ports[peer_id];
				if(stub.peer_sockets.count(peer_id) == 0){
					ServerSocket temp_socket;
					stub.peer_sockets.insert(std::pair<int,ServerSocket>(peer_id, temp_socket));
				}
				//Add peer alive code here
				if(peer_isalive[peer_id]){
					if (!stub.peer_sockets[peer_id].Init(peer_ip, port)) {
						std::cout << "Server id " << peer_id << " failed to connect, Backup Server down: "<< peer_id << std::endl;
						peer_isalive[peer_id]= false;
					}
					else{
						stub.SendAdminId(&stub.peer_sockets[peer_id], unique_id);
					}

				}

			}
			connection_flag = true;
		}
		ul.unlock();

		smr_obj.opcode = -10;
		smr_obj.arg1 = -10;
		smr_obj.arg2 = -10;

		rep_record.SetRecord(-10, -10, -10, smr_obj);

		timer_lock.lock();

		if(elapsed_time > adjusted_timeout){
			for (auto const& peer_info : peer_isalive){
				int peer_id = peer_info.first;
				bool peer_alive = peer_info.second;


						if(peer_alive){
								if(!stub.SendReplicationRequest(&stub.peer_sockets[peer_id], rep_record)){
									printf("Follower %d Down\n", peer_id);
									peer_isalive[peer_id] = false;
								 }

								if(!stub.ReceiveReplicationAck(&stub.peer_sockets[peer_id])){
									 printf("Follower %d Down\n", peer_id);
	 								 peer_isalive[peer_id] = false;
								}
						}
				}
				printf("Sent HeartBeat\n");
				start_time = std::chrono::high_resolution_clock::now();
		}

		timer_lock.unlock();

	}

}

void LaptopFactory::ElectionThread() {
	ServerStub stub;
	std::this_thread::sleep_for(std::chrono::microseconds(10000000));
	while(true){

		timer_lock.lock();
		if((std::chrono::high_resolution_clock::now() - start_time) > timeout) {
			// term_number++;
			// std::this_thread::sleep_for(std::chrono::microseconds(rand() % 100));
			// int n = 0;
			// for(PeerServer peer: peer_vector) {
			// 	if(peer.is_up == 1) {
			// 		if(stub.Connect(peer, i) != 0){
			// 			stub.SendIdent(term_number, n)
			// 		};
			// 	}
			// 	n++;
			// }
			// std::cout<<std::chrono::duration_cast<std::chrono::microseconds>(start_time).count()<<std::endl;
			std::cout << "Leader Election started: " << unique_id << std::endl;
			break;
		}
		timer_lock.unlock();
	}
}
