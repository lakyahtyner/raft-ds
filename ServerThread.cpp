#include <iostream>
#include <memory>
#include "randutils.hpp"

#include "ServerThread.h"
#include "ServerStub.h"

std::chrono::duration<double, std::micro> timeout = std::chrono::microseconds(8000000);


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


ServerStub LaptopFactory::MakeConnections(){
	ServerStub stub;
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
	return stub;
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

	stub.Init(std::move(socket));
	IFA_id = stub.ReceiveId();

	// printf("Primary id %d\n", IFA_id);

	while (true) {

		if(IFA_id>=0){
					// printf("Inside Replication Function\n");
					rep_record = stub.ReceiveReplicationRequest();

					//check if Leader went down Factory ID = -1
					if(rep_record.GetFactoryId() == -1){
						leader_lock.lock();
						peer_isalive[IFA_id] = false;
						primary_id = -1;
						leader_id = -1;
						cast_vote = false;
						printf("Leader went down, Starting Election \n");
						leader_cv.notify_all();
						leader_lock.unlock();
						break;
					}

					//HeartBeat check uses Factory ID = -10
					else if(rep_record.GetFactoryId() == -10){
						printf("Recieved HeartBeat\n");
						start_time = std::chrono::high_resolution_clock::now();
						stub.SendReplicationAck();
						continue;
					}

					//Leader Election check GetFactoryId = -5
					else if(rep_record.GetFactoryId() == -5){
						vote_lock.lock();
						if(!cast_vote){
							cast_vote = true;
							//Casting a vote for the first Candidate.
							// printf("Casting Candidate vote\n");
							stub.SendReplicationAck(1);
						}

						else{
							// printf("Already Casted vote\n");
							stub.SendReplicationAck(0);
						}
						vote_lock.unlock();

						leader_lock.lock();
						// printf("Waiting for reply\n");
						int candidate_id = stub.ReceiveId();
						// printf("candidate_id %d\n", candidate_id);

						if(candidate_id > -1){
							// printf("candidate_id inside loop %d\n", candidate_id);

							leader_id = candidate_id;
							printf("The new leader is %d\n",leader_id);
							// leader_cv.notify_all();
						}

						// printf("leader_id %d\n", leader_id);
						// printf("Breaking from the loop\n");
						leader_lock.unlock();

						break;
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

					map_lock.unlock();

					stub.SendReplicationAck();

		}

		else{
			order = stub.ReceiveRequest();

			if (!order.IsValid()) {
				break;
			}
			request_type = order.GetRequestType();

			if((unique_id != leader_id)&&(request_type == 1)){
				break;
			}

			switch (request_type) { // harsh: build request_type functions.
				case 1:
						laptop = CreateLaptop(order, engineer_id);
						stub.SendLaptop(laptop);
					break;
				case 2:
					record = AccessCustomerMap(order);
					stub.ReturnRecord(record);
					stub.SendLeaderId(leader_id);
					break;
				// case 3:
				// 	// printf("Sending leader id\n");
				// 	leader_lock.lock();
				// 	stub.SendLeaderId(leader_id);
				// 	leader_lock.unlock();
				// 	break;
				default:
					std::cout << "Undefined request type: "<< request_type << std::endl;

			}
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

			stub = MakeConnections();
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
	// std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	// printf("Instantiated Heart Beat Thread\n");
	MapOp smr_obj;
	bool connection_flag = false;
	ServerStub stub;
	ReplicationRecord rep_record;
	std::unique_lock<std::mutex> ul(leader_lock, std::defer_lock);

	std::chrono::time_point<std::chrono::high_resolution_clock> curr;
	std::chrono::duration<double> elapsed_time, adjusted_timeout;
	adjusted_timeout = timeout - std::chrono::microseconds(10000);

	while (true) {

		if (leader_id !=unique_id) {
			leader_cv.wait(ul, [this]{ return (leader_id == unique_id); });
		}


			// if(leader_id == unique_id){

			curr = std::chrono::high_resolution_clock::now();
			elapsed_time = curr - start_time;

			// ul.lock();
			if(!connection_flag){
				stub = MakeConnections();
				connection_flag = true;
			}
			// ul.unlock();

			smr_obj.opcode = -10;
			smr_obj.arg1 = -10;
			smr_obj.arg2 = -10;

			rep_record.SetRecord(-10, -10, -10, smr_obj);

			// timer_lock.lock();

			if(elapsed_time > adjusted_timeout){
				for (auto const& peer_info : peer_isalive){
					int peer_id = peer_info.first;
					bool peer_alive = peer_info.second;

							if(peer_alive){
									if(!stub.SendReplicationRequest(&stub.peer_sockets[peer_id], rep_record)){
										printf("Follower %d Down\n", peer_id);
										peer_isalive[peer_id] = false;
									 }

									if(stub.ReceiveReplicationAck(&stub.peer_sockets[peer_id]) == -1){
										 printf("Follower %d Down\n", peer_id);
		 								 peer_isalive[peer_id] = false;
									}
							}
					}
					printf("HeartBeat Sent\n");
					start_time = std::chrono::high_resolution_clock::now();
			}

			// timer_lock.unlock();
		// }

	}

}

void LaptopFactory::ElectionThread() {
	ServerStub stub;
	MapOp smr_obj;
	ReplicationRecord rep_record;
	// bool connection_flag = false;
	std::unique_lock<std::mutex> ul(leader_lock, std::defer_lock);

	// printf("ElectionThread Instantiated\n");
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	while(true){
		// if(leader_id == -1){
		//I dont think this is needed, the timer should make the election start
			// if (leader_id > -1) {
				// leader_cv.wait(ul, [this]{ return (leader_id == -1); });
			// }

			// timer_lock.lock();

				if((std::chrono::high_resolution_clock::now() - start_time) > timeout) {
					// printf("ElectionStarted\n");
					int total_votes = 0;
					int alive_peers = 0;
					ServerStub stub;
					randutils::mt19937_rng rng;

	 				// std::cout << rng.uniform(1,1000) << ": Random Timer\n";
					std::this_thread::sleep_for(std::chrono::milliseconds(int(rng.uniform(1,1000))));
					// std::this_thread::sleep_for(std::chrono::milliseconds(10*unique_id));
					// printf("Candidate Node Awake for Election \n");

					// if(unique_id == 1){
					// 		std::this_thread::sleep_for(std::chrono::milliseconds(1*unique_id));
					// 		printf("Candidate Node Awake for Election \n");
					// }
					// else{
					// 		std::this_thread::sleep_for(std::chrono::milliseconds(50));
					// 		printf("Candidate Node Awake for Election \n");
					// }
					// std::this_thread::sleep_for(std::chrono::microseconds((rand()*500) % 5000000));

					// if(!connection_flag){
					stub = MakeConnections();
						// connection_flag = true;
					// }

					smr_obj.opcode = -5; smr_obj.arg1 = -5; smr_obj.arg2 = -5;
					rep_record.SetRecord(-5, -5, -5, smr_obj);



					vote_lock.lock();

					if(!cast_vote){
						cast_vote = true;
						//Casting a vote for tSelf.
						// printf("Casting Candidate vote to self\n");
						total_votes = total_votes + 1;
					}
					else{
						// printf("Already Casted vote to a peer\n");
					}

					vote_lock.unlock();

					for (auto const& peer_info : peer_isalive){
						int peer_id = peer_info.first;
						bool peer_alive = peer_info.second;

								if(peer_alive){
										if(!stub.SendReplicationRequest(&stub.peer_sockets[peer_id], rep_record)){
											printf("Replication request: Node %d Down\n", peer_id);
											peer_isalive[peer_id] = false;
										 }

										 int vote_ack = stub.ReceiveReplicationAck(&stub.peer_sockets[peer_id]);

										if(vote_ack == -1){
											 printf("Voteack:Node %d Down\n", peer_id);
			 								 peer_isalive[peer_id] = false;
										}

										else{
											total_votes = total_votes + vote_ack;
										}

										alive_peers = alive_peers + int(peer_isalive[peer_id]);
								}
						}

						//Broadcast leader id to everyone
						leader_lock.lock();
						// printf("Alive Peers %d\n", alive_peers);
						if(2*total_votes-1 > alive_peers){
							printf("I am the Leader\n");
							printf("Total Votes: %d\n",total_votes);

							for (auto const& peer_info : peer_isalive){
									int peer_id = peer_info.first;
									bool peer_alive = peer_info.second;

									if(peer_alive){
										stub.SendAdminId(&stub.peer_sockets[peer_id], unique_id);
									}
							}

							leader_id = unique_id;
							// leader_cv.notify_all();
						}
						else{

							for (auto const& peer_info : peer_isalive){
									int peer_id = peer_info.first;
									bool peer_alive = peer_info.second;

									if(peer_alive){
										stub.SendAdminId(&stub.peer_sockets[peer_id], -1);
									}
							}

						}
						start_time = std::chrono::high_resolution_clock::now();
						leader_cv.notify_all();
						leader_lock.unlock();

						// printf("Total Alive Peers %d\n",alive_peers);

						// break;
				}
			// timer_lock.unlock();
		// }
	}
}
