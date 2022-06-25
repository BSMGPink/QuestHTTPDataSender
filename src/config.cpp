#include "config.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

config_t config;

Configuration& getConfig() {
	static Configuration config({ID, VERSION});
	config.Load();
	return config;
}

#define SAVE(name) \
	doc.AddMember(#name, config.name, allocator)

#define LOAD(name, method) \
	auto itr_ ##name = doc.FindMember(#name); \
	if (itr_ ##name != doc.MemberEnd()) { \
		config.name = itr_ ##name->value.method; \
	}else { foundEverything = false; } 

void SaveConfig()
{
	INFO("Saving Configuration...");
	rapidjson::Document& doc = getConfig().config;
	doc.RemoveAllMembers();
	doc.SetObject();
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
	
	SAVE(dataSendType);
	SAVE(sendRequests);
	SAVE(gamerTag);
	SAVE(gamerTagSearchURL);
	SAVE(httpReqeustURL);
	SAVE(gamerTagCheckRequirement);
	SAVE(requestTimeoutTime);



	getConfig().Write();
	INFO("Saved Configuration!");
}

bool LoadConfig()
{
	INFO("Loading Configuration...");
	bool foundEverything = true;
	rapidjson::Document& doc = getConfig().config;

	LOAD(dataSendType, GetInt());
	LOAD(gamerTag, GetString());
	LOAD(gamerTagSearchURL, GetString());
	LOAD(httpReqeustURL, GetString());
	LOAD(gamerTagCheckRequirement, GetBool());
	LOAD(sendRequests, GetBool());
	LOAD(requestTimeoutTime, GetFloat());

	if (!foundEverything) 
	{
		ERROR("Configuration values were missing!");
		return false;
	}
	INFO("Loaded Configuration!");
	return true;
}
//taken from PinkCore