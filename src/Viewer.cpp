#include "Viewer.h"
#include "display_descriptor.h"

#include <json/json.h>

#include <iostream>

namespace OSVRCardboard {
	Viewer::Viewer() {}
	Viewer::~Viewer() {}

	bool Viewer::parseFromJson(Json::Value config) {
		if (!config.isMember("viewerParams") ||
			!config.isMember("deviceWidth") ||
			!config.isMember("screenWidth") ||
			!config.isMember("screenHeight") ||
			!config.isMember("screenHorizontal") ||
			!config.isMember("screenVertical") ||
			!config.isMember("deviceName") ||
			!config.isMember("protocolVersion"))
		{
			throw std::bad_alloc();
		}

		m_viewer_params = config["viewerParams"].asString();
		std::string deviceString = base64_decode(m_viewer_params);
		if (!m_device.ParseFromString(deviceString)) {
			throw std::bad_alloc();
		}
		m_device_width = config["deviceWidth"].asFloat();
		m_screen_width = config["screenWidth"].asFloat();
		m_screen_height = config["screenHeight"].asFloat();
		m_screen_horizontal = config["screenHorizontal"].asInt();
		m_screen_vertical = config["screenVertical"].asInt();
		m_device_name = config["deviceName"].asString();

		return true;
	}

	std::string Viewer::viewerParams()
	{
		return m_viewer_params;
	}

	std::string Viewer::name()
	{
		return m_device_name;
	}

	std::string Viewer::vendor()
	{
		return m_device.vendor();
	}

	std::string Viewer::model()
	{
		return m_device.model();
	}

	void Viewer::resolution(unsigned long *resolution)
	{
		resolution[0] = m_screen_vertical;
		resolution[1] = m_screen_horizontal;
		resolution[2] = 32;
	}

	std::string Viewer::displayConfig()
	{
		Json::Value config;
		Json::Reader reader;
		double degreesPerRadian = 180.0f / 3.14159f;

		reader.parse(display_descriptor, config);

		if (m_device.has_vendor()) {
			config["hmd"]["device"]["vendor"] = m_device.vendor();
		}
		if (m_device.has_model()) {
			config["hmd"]["device"]["model"] = m_device.model();
		}

		config["hmd"]["resolutions"][0]["width"] = m_screen_vertical;
		config["hmd"]["resolutions"][0]["height"] = m_screen_horizontal;

		if (m_device.has_inter_lens_distance()) {
			double center_proj_x = (m_screen_height - m_device.inter_lens_distance()) / m_screen_height;
			config["hmd"]["eyes"][0]["center_proj_x"] = center_proj_x;
			config["hmd"]["eyes"][1]["center_proj_x"] = 1.0f - center_proj_x;

			if (m_device.has_tray_to_lens_distance()) {
				double fov_left = atan((center_proj_x * m_screen_height / 2) / m_device.tray_to_lens_distance());
				double fov_right = atan(((1.0f - center_proj_x) * m_screen_height / 2) / m_device.tray_to_lens_distance());

				config["hmd"]["field_of_view"]["monocular_horizontal"] = (fov_left + fov_right) * degreesPerRadian;
			}
		}
		if (m_device.has_tray_to_lens_distance()) {
			double center_proj_y = (m_device.tray_to_lens_distance() - ((m_device_width - m_screen_width) / 2)) / m_screen_width;

			if (m_device.has_vertical_alignment()) {
				switch (m_device.vertical_alignment()) {
				case DeviceParams::BOTTOM:
					break;
				case DeviceParams::TOP:
					center_proj_y = 1.0f - center_proj_y;
					break;
				case DeviceParams::CENTER:
					center_proj_y = 0.5f;
					break;
				}

			}
			config["hmd"]["eyes"][0]["center_proj_y"] = center_proj_y;
			config["hmd"]["eyes"][1]["center_proj_y"] = center_proj_y;

			double fov_bottom = atan((center_proj_y * m_screen_width) / m_device.tray_to_lens_distance());
			double fov_top = atan(((1.0f - center_proj_y) * m_screen_width) / m_device.tray_to_lens_distance());

			config["hmd"]["field_of_view"]["monocular_vertical"] = (fov_top + fov_bottom) * degreesPerRadian;
		}

		if (m_device.distortion_coefficients_size()) {
			Json::Value distortion = Json::Value(Json::arrayValue);
			distortion.append(0.0f);
			distortion.append(1.0f);
			for (int i = 0; i < m_device.distortion_coefficients_size(); i++) {
				distortion.append(m_device.distortion_coefficients(i));
			}
			config["hmd"]["distortion"]["polynomial_coeffs_red"] = distortion;
			config["hmd"]["distortion"]["polynomial_coeffs_green"] = distortion;
			config["hmd"]["distortion"]["polynomial_coeffs_blue"] = distortion;
		}

		Json::StyledWriter writer;

		return writer.write(config);
	}

	bool Viewer::hasMagnet()
	{
		return m_device.has_magnet();
	}

/*
	base64.cpp and base64.h

	Copyright (C) 2004-2008 René Nyffenegger

	This source code is provided 'as-is', without any express or implied
	warranty. In no event will the author be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this source code must not be misrepresented; you must not
	claim that you wrote the original source code. If you use this source code
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original source code.

	3. This notice may not be removed or altered from any source distribution.

	René Nyffenegger rene.nyffenegger@adp-gmbh.ch

	-----

	Source below has been altered by <steve@nourish.je> to fit the application
*/

	const std::string Viewer::base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789-_";

	bool Viewer::is_base64(unsigned char c) {
		return (isalnum(c) || (c == '-') || (c == '_'));
	}

	std::string Viewer::base64_decode(std::string const& encoded_string) {
		int in_len = encoded_string.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];
		std::string ret;

		while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
			char_array_4[i++] = encoded_string[in_]; in_++;
			if (i == 4) {
				for (i = 0; i <4; i++)
					char_array_4[i] = base64_chars.find(char_array_4[i]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					ret += char_array_3[i];
				i = 0;
			}
		}

		if (i) {
			for (j = i; j <4; j++)
				char_array_4[j] = 0;

			for (j = 0; j <4; j++)
				char_array_4[j] = base64_chars.find(char_array_4[j]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
		}

		return ret;
	}
}