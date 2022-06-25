#pragma once
#include <string>

typedef struct config_t {
	int dataSendType = 0;
	bool gamerTagCheckRequirement = true;
	bool sendRequests = true;
	float requestTimeoutTime = 5.0f;
	std::string gamerTag = "";
	std::string gamerTagSearchURL = "https://bomberman-middleware.herokuapp.com/api/v1/zbd/check-gamertag/";
	std::string httpReqeustURL = "https://bootygeo.herokuapp.com/api/v1/beatsaber/hit";
} config_t;

extern config_t config;

bool LoadConfig();
void SaveConfig();