#include "UI/Settings/DataSenderSettingsViewController.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "config.hpp"
#include "logging.hpp"

DEFINE_TYPE(DataSender::UI, DataSenderSettingsViewController);

using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;

extern HMUI::InputFieldView* gamerTagTextBox;

namespace DataSender::UI
{
	void DataSenderSettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
	{
		if (firstActivation)
		{
			GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(get_transform());

			QuestUI::BeatSaberUI::CreateToggle(container->get_transform(), "Send Data", config.sendRequests, [](bool value){
				config.sendRequests = value; 
				SaveConfig(); 
			});

			QuestUI::BeatSaberUI::CreateToggle(container->get_transform(), "Require GamerTag Check", config.gamerTagCheckRequirement, [](bool value){
				config.gamerTagCheckRequirement = value; 
				SaveConfig(); 
			});
		
            std::vector<StringW> dataSendTypeStringWVector = { 
                "Don't Limit",
                "Wait For Previous Request",
                "Only Every 2nd Note",
                "Only Every 4th Note"
            };
            StringW dataSendTypeStringW = dataSendTypeStringWVector[config.dataSendType];
            
            QuestUI::BeatSaberUI::CreateDropdown(container->get_transform(), "Limit Requests By", dataSendTypeStringW, dataSendTypeStringWVector,
                [dataSendTypeStringWVector](std::string value) {
                    if (value == dataSendTypeStringWVector[0]) {
                        config.dataSendType = 0;
                    }else if (value == dataSendTypeStringWVector[1]) {
                        config.dataSendType = 1;
                    }else if (value == dataSendTypeStringWVector[2]) {
                        config.dataSendType = 2;
                    }else {
                        config.dataSendType = 3;
					}
                    SaveConfig();
                }
            );
            
            QuestUI::BeatSaberUI::CreateIncrementSetting(container->get_transform(), "Request Timeout Length", 1, 0.25f, config.requestTimeoutTime, 1.0f, 10.0f, 
                [](float value) { 
                    config.requestTimeoutTime = value; 
                    SaveConfig(); 
                } 
            );
			QuestUI::BeatSaberUI::CreateUIButton(container->get_transform(), "Reset Username", [](){
				config.gamerTag = ""; 
				if(gamerTagTextBox != nullptr)
					gamerTagTextBox->SetText("");
				SaveConfig(); 
			});

				
		}
	}
}