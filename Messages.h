#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>

struct MapOp {
	int opcode;
	int arg1;
	int arg2;
};

class Request {
private:
	int customer_id;
	int order_number;
	int request_type;

public:
	Request();
	void operator = (const Request &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	void SetRequest(int cid, int order_num, int type);
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class ReplicationRequest {
private:
	int factory_id;
	int last_index;
	int committed_index;
	MapOp updt;

public:
	ReplicationRequest();
	void operator = (const ReplicationRequest &rprqst) {
		factory_id = rprqst.factory_id;
		last_index = rprqst.last_index;
		committed_index	 = rprqst.committed_index;
		updt = rprqst.updt;
	}
	void SetRequest(int fid, int lidx, int cidx, MapOp updmp);
	int GetFactoryId();
	int GetLastIndex();
	int GetCommittedIndex();
	MapOp GetUpdateMap();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class LaptopInfo {
private:
	int customer_id;
	int order_number;
	int request_type;
	int engineer_id;
	int admin_id;

public:
	LaptopInfo();
	void operator = (const LaptopInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int admid);
	void CopyOrder(Request order);
	void SetEngineerId(int id);
	void SetAdminId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();
	int GetEngineerId();
	int GetAdminId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class CustomerRecord {
private:
	int customer_id;
	int last_order;

public:
	CustomerRecord();
	void operator = (const CustomerRecord &record) {
		customer_id = record.customer_id;
		last_order = record.last_order;
	}
	void SetRecord(int cid, int lst_odr);
	int GetCustomerId();
	int GetLastOrder();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

void MarshalIdent(char *buffer, int ident);
int UnmarshalIdent(char *buffer);

#endif // #ifndef __MESSAGES_H__
