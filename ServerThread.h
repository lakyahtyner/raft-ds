#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include<map>


#include "Messages.h"
#include "ServerSocket.h"

struct ExpertRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
};

class LaptopFactory {
private:
	std::queue<std::unique_ptr<ExpertRequest>> erq;
	std::mutex erq_lock;
	std::condition_variable erq_cv;

	//harsh: New datastructures
	std::map<int, int> customer_map;
	std::vector<MapOp> smr_log;
	std::mutex log_lock;
	std::mutex map_lock;
	std::condition_variable map_cv;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	std::mutex timer_lock;
	// std::condition_variable timer_cv;




	CustomerRecord AccessCustomerMap(CustomerRequest order);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id);
public:
	static int unique_id; //this is the same as factory_id
	static int n_peers;
	static std::map<int,bool> peer_isalive;
	static std::map<int,std::string> peer_ips;
	static std::map<int,int> peer_ports;
	static int leader_id; //Leader Id for Raft

	int last_index = -1;
	int committed_index = -1;
	int primary_id = -1;
	// int factory_id = -1;
	void EngineerThread(std::unique_ptr<ServerSocket> socket, int id);
	void AdminThread(int id);
	void HeartbeatThread();
	void ElectionThread();
};

#endif // end of #ifndef __SERVERTHREAD_H__
