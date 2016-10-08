#pragma once

#define _WINSOCKAPI_

#include <windows.h>
#include <winsock2.h>

#include <thread>
#include <mutex>
#include <queue>

#include "Viewer.h"
#include "osvr/Util/QuaternionC.h"
#include "osvr/Util/TimeValueC.h"

#pragma comment(lib,"ws2_32.lib")

#define TS_BUFFER_SIZE 1025
#define OSVR_CARDBOARD_PORT 5555
#define OSVR_CARDBOARD_DELIMITER "\n"

#define SET_STATUS(data, status, message) (data).mutex.lock(); (data).ready = (status); (data).statusMessage = (message); (data).mutex.unlock();
#define SET_ERROR(data, message) (data).mutex.lock(); (data).ready = false; (data).error = true; (data).statusMessage = (data).errorMessage = (message); (data).mutex.unlock();

namespace OSVRCardboard {
	struct TimestampedQuaternion {
		OSVR_Quaternion quaternion;
		OSVR_TimeValue timestamp;
	};

	struct net_thread_data
	{
		std::mutex mutex;
		bool end = false;
		bool disconnect = false;
		Viewer config;
		bool configChanged = false;
		char* statusMessage = "";
		char* errorMessage = "";
		bool ready = false;
		bool error = false;
		std::queue<TimestampedQuaternion> quaternions;
	};



	class TrackingServer {
	public:
		TrackingServer();
		~TrackingServer();

		Viewer config();
		bool hasQuaternion();
		TimestampedQuaternion quaternion();

		bool configChanged();
		bool hasError();
		bool isReady();
		char* getError();
		char* getStatus();

		void disconnect();

		static void net_thread(net_thread_data& data);
	private:
		std::thread* m_net_thread;
		net_thread_data m_net_thread_data;


	};
}