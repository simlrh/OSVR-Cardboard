#pragma once

#include <string>
#include <json/json.h>
#include "CardboardDevice.pb.h"

namespace OSVRCardboard {
	class Viewer {
	public:
		Viewer();
		~Viewer();

		bool parseFromJson(Json::Value config);

		std::string name();
		std::string vendor();
		std::string model();
		bool hasMagnet();
		void resolution(unsigned long *resolution);

		std::string displayConfig();

		std::string viewerParams();
	private:
		DeviceParams m_device;
		float m_device_width;
		float m_screen_width;
		float m_screen_height;
		int m_screen_horizontal;
		int m_screen_vertical;
		std::string m_device_name;
		std::string m_viewer_params;

		static const std::string base64_chars;
		static inline bool is_base64(unsigned char c);
		std::string base64_decode(std::string const& encoded_string);
	};
}