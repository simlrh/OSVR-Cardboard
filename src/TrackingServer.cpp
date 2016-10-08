#include "TrackingServer.h"

#include <iostream>
#include <json/json.h>

namespace OSVRCardboard {

	TrackingServer::TrackingServer()
	{
		m_net_thread = new std::thread(TrackingServer::net_thread, std::ref(m_net_thread_data));
	}

	Viewer TrackingServer::config()
	{
		std::lock_guard<std::mutex> lock(m_net_thread_data.mutex);
		return m_net_thread_data.config;
	}

	bool TrackingServer::hasQuaternion()
	{
		std::lock_guard<std::mutex> lock(m_net_thread_data.mutex);
		return !m_net_thread_data.quaternions.empty();
	}

	TimestampedQuaternion TrackingServer::quaternion()
	{
		std::lock_guard<std::mutex> lock(m_net_thread_data.mutex);
		TimestampedQuaternion q = m_net_thread_data.quaternions.front();
		m_net_thread_data.quaternions.pop();
		return q;
	}

	bool TrackingServer::configChanged()
	{
		bool ret = m_net_thread_data.configChanged;
		m_net_thread_data.configChanged = false;
		return ret;
	}

	void TrackingServer::disconnect()
	{
		std::lock_guard<std::mutex> lock(m_net_thread_data.mutex);
		m_net_thread_data.disconnect = true;
	}

	void TrackingServer::net_thread(net_thread_data& data)
	{
		SET_STATUS(data, false, "Initialising networking");

		WSADATA WsaDat;
		if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
		{
			SET_ERROR(data, "Network initialization failed");
			WSACleanup();
			return;
		}

		SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (Socket == INVALID_SOCKET)
		{
			SET_ERROR(data, "Network initialization failed");
			WSACleanup();
			return;
		}

		SOCKADDR_IN serverInf;
		serverInf.sin_family = AF_INET;
		serverInf.sin_addr.s_addr = INADDR_ANY;
		serverInf.sin_port = htons(OSVR_CARDBOARD_PORT);

		if (bind(Socket, (SOCKADDR*)(&serverInf), sizeof(serverInf)) == SOCKET_ERROR)
		{
			SET_ERROR(data, "Network initialization failed");
			WSACleanup();
			return;
		}

		listen(Socket, 1);

		bool end = false;
		do {

			SET_STATUS(data, false, "Waiting for connection");

			SOCKET ClientSocket = SOCKET_ERROR;
			while (ClientSocket == SOCKET_ERROR)
			{
				ClientSocket = accept(Socket, NULL, NULL);
			}

			SET_STATUS(data, true, "Client connected");

			int received = 0;
			char report[TS_BUFFER_SIZE];
			char sendBuffer[TS_BUFFER_SIZE];
			do {
				if (data.end || data.disconnect) {
					break;
				}

				received = recv(ClientSocket, (char*)report, TS_BUFFER_SIZE - 1, 0);
				if (received == SOCKET_ERROR) break;

				report[received] = 0;
				
				double x, y, z, w;
				OSVR_TimeValue_Seconds s;
				OSVR_TimeValue_Microseconds m;

				char* context;
				char* lineptr = strtok_s(report, OSVR_CARDBOARD_DELIMITER, &context);
				while (lineptr) {
					//printf("%s\n", lineptr);
					// Orientation report
					if (6 == sscanf_s(lineptr, "{\"x\":%lf,\"y\":%lf,\"z\":%lf,\"w\":%lf,\"s\":%lld,\"m\":%ld}", &x, &y, &z, &w, &s, &m)) {
						TimestampedQuaternion q;
						osvrQuatSetX(&q.quaternion, x);
						osvrQuatSetY(&q.quaternion, y);
						osvrQuatSetZ(&q.quaternion, z);
						osvrQuatSetW(&q.quaternion, w);
						q.timestamp.seconds = s;
						q.timestamp.microseconds = m;
						data.mutex.lock();
						data.quaternions.push(q);
						data.mutex.unlock();
					}
					// Clock synchronistion
					else if (2 == sscanf_s(lineptr, "{\"s\":%lld,\"m\":%ld}", &s, &m)) {
						OSVR_TimeValue timeValue;
						osvrTimeValueGetNow(&timeValue);
						int sent = sprintf_s(sendBuffer, TS_BUFFER_SIZE, "{\"s\":%lld,\"m\":%ld,\"ss\":%lld,\"sm\":%ld}\n", s, m, timeValue.seconds, timeValue.microseconds);
						send(ClientSocket, sendBuffer, sent, 0);
					}
					// Config object
					else {
						Json::Value configJson;
						Json::Reader reader;
						bool parsed;

						parsed = reader.parse(report, configJson);

						if (parsed && configJson.isObject() && configJson.isMember("viewerParams")) {
								try {
									data.mutex.lock();
									data.config.parseFromJson(configJson);
									data.configChanged = true;
									data.mutex.unlock();
								}
								catch (const std::bad_alloc& e) {
									std::cout << "Bad config: " << report << std::endl;
								}
						}
					}
					lineptr = strtok_s(NULL, OSVR_CARDBOARD_DELIMITER, &context);
				}
				
			} while (received != 0);

			SET_STATUS(data, true, "Client disconnected");

			shutdown(ClientSocket, SB_BOTH);
			closesocket(ClientSocket);
			data.disconnect = false;

		} while (!data.end);

		WSACleanup();
	}

	TrackingServer::~TrackingServer()
	{
		m_net_thread_data.end = true;
		m_net_thread->join();
	}


	bool TrackingServer::hasError()
	{
		return m_net_thread_data.error;
	}

	char* TrackingServer::getError()
	{
		return m_net_thread_data.errorMessage;
	}

	bool TrackingServer::isReady()
	{
		return m_net_thread_data.ready;
	}

	char* TrackingServer::getStatus()
	{
		return m_net_thread_data.statusMessage;
	}

}