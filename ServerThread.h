#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <map>
#include <chrono>

#include "Messages.h"
#include "ServerSocket.h"
#include "ServerStub.h"

struct AdminRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
};

class LaptopFactory {
	private:
		std::vector<PeerServer> peer_vector;

		std::queue<std::unique_ptr<AdminRequest>> erq;
		std::mutex erq_lock;
		std::condition_variable erq_cv;

		std::mutex smr_lock;
		std::condition_variable smr_cv;
		std::vector<MapOp> smr_log;

		std::mutex map_lock;
		std::condition_variable map_cv;
		std::map<int, int> customer_map;

		int last_index;
		int committed_index = 0;
		int primary_id; 
		int factory_id;

		std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
		//std::chrono::duration<double, std::micro> elapsed_time;

		CustomerRecord GetCustomerRecord(Request order);
		LaptopInfo CreateLaptop(Request order, int engineer_id);

	public:
		void EngineerThread(std::unique_ptr<ServerSocket> socket, int id);
		void ProductionAdminThread(int id, int uid);
		void SendHeartbeatThread();
		void AddPeer(PeerServer new_peer);
};

#endif // end of #ifndef __SERVERTHREAD_H__

