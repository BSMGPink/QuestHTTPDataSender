#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "hooks.hpp"
#include "config.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/GameScenesManager.hpp"
#include "GlobalNamespace/GameEnergyCounter.hpp"
#include "GlobalNamespace/CoreGameHUDController.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Coroutine.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "System/Action.hpp"
#include "System/Action_1.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "Utils/WebUtils.hpp"
#include "logging.hpp"

#include "libcurl/shared/curl.h"
#include "libcurl/shared/easy.h"

#include <thread>
#include "static-defines.h"

int goodHitCounter = 0;
int badHitCounter = 0;

bool requestInProgress = false;


TMPro::TextMeshProUGUI* overheadText;



custom_types::Helpers::Coroutine FadeTextToFullAlpha(float t) //c# coroutine
{
    if(!overheadText) //if the text is null (doesnt exist yet)
        co_return; //return, the code below would crash as the text doesnt exist yet
    UnityEngine::Color colour = overheadText->get_color();
    while(overheadText->get_color().a < 1.0){
        if(!overheadText)
            break;
        overheadText->set_color({colour.r,colour.g,colour.b, overheadText->get_color().a + (UnityEngine::Time::get_deltaTime() / t)}); //increase the opacity
        co_yield nullptr; //wait
    }
    co_return;
} 


void PostRequest(bool setText){

    if((!config.sendRequests) || (config.dataSendType == 1 && requestInProgress)) 
        return; //if the mod is disabled, or the setting is applied to only send requests when another request has finished, and a request is in progress, stop the method from running

    requestInProgress = true;

    WebUtils::PostAsync(config.httpReqeustURL, (long)config.requestTimeoutTime, [&](long code, std::string result){
        rapidjson::Document d; //create a new json document
        switch (code)
        {
            case 200: //if we get a valid responce
                d.Parse(result); //parse it
                break;
            default:
                d.Parse("{\"message\":\"ConnectionError\"}"); //parse our own error message to handle below
                break;
        }
        
		if (d.HasMember("message")) { //if the json document has the 'message' key, ensures that it parsed correctly
            std::string message = d["message"].GetString();
            if(message == "Success"){
                INFO("Success: %s", result.c_str());
            }else if(message == "ConnectionError"){
                setText = false;
                INFO("Failure: Internet Connection Error");
            }else{
                setText = false;
                INFO("Failure: Data Error, Not A Success");
                return;
            }//log info about the request and stop any text from being updated if the request failed
            if(setText && d.HasMember("data")){ //if the json has the 'data' key
                std::string score = d["data"]["score"].GetString();
                QuestUI::MainThreadScheduler::Schedule([score] { //run below on the start of the next frame, as we're messing with objects on another thread, we need to make sure we dont change anything in the middle of a frame.
                    if(overheadText){
                        if(overheadText->get_color().a == 0.0){ //the text starts with a transparency of 0, this only runs on first request
                            GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FadeTextToFullAlpha(0.6))); //start fading the text in
                        }
                        overheadText->SetText(config.gamerTag + "  :  " + score); //set the text, would use | but the font doesnt support it
                    }
                    
                });
            }
        }else{
            INFO("Failure: Data Error, No Message Key");
        }
        requestInProgress = false;
        
    }, nullptr, "gamerTag=" + config.gamerTag + "&hitCounter=" + std::to_string(goodHitCounter)  + "&missCounter=" + std::to_string(badHitCounter));
    
}

int notehit = 0;
MAKE_AUTO_HOOK_MATCH(ScoreController_HandleNoteWasCut, &GlobalNamespace::ScoreController::HandleNoteWasCut, void, GlobalNamespace::ScoreController* self, GlobalNamespace::NoteController* noteController,  ByRef<GlobalNamespace::NoteCutInfo> noteCutInfo)
{
    //note cut method, gets run whenever a note is cut in any gamemode

    if(!overheadText){ //if the text is null
        auto transform = QuestUI::ArrayUtil::Last(UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ComboUIController*>())->get_transform();
        overheadText = QuestUI::BeatSaberUI::CreateText(transform, "", UnityEngine::Vector2(0, 0));
        overheadText->set_alignment(TMPro::TextAlignmentOptions::Center);
        overheadText->get_transform()->set_localScale({6.0f, 6.0f, 0.0f});
        overheadText->set_color({1.0,1.0,1.0,0.0});
        overheadText->set_fontStyle(TMPro::FontStyles::Normal); //create the text and move it into position
        overheadText->get_transform()->set_position({0,3,8});
    }

    ScoreController_HandleNoteWasCut(self, noteController, noteCutInfo); //run base game method ScoreController_HandleNoteWasCut

    if(notehit == 0){
        if(noteCutInfo->get_allIsOK()){ //if the note was cut correctly
            goodHitCounter++;
        }else{ //if the note was cut incorrectly (wrong colour, wrong direction)
            badHitCounter++;
        }
        PostRequest(true); //send the http request and update the text
    }
    if(config.dataSendType > 1)
        notehit++;

    if((config.dataSendType == 2 && notehit == 1) || (config.dataSendType == 3 && notehit == 3)){
        notehit = 0;
    }

}

MAKE_AUTO_HOOK_MATCH(GameScenesManager_PushScenes, &GlobalNamespace::GameScenesManager::PushScenes, void, GlobalNamespace::GameScenesManager* self, GlobalNamespace::ScenesTransitionSetupDataSO* scenesTransitionSetupData, float minDuration, System::Action* afterMinDurationCallback, System::Action_1<::Zenject::DiContainer*>* finishCallback)
{
    //this method calls as soon as the game loads 

    if(overheadText){ //if the text exists
        overheadText = nullptr; //set it to null, this would have been destroyed by the GC, this just allows the note cut hook we made to create another one. 
    }

    GameScenesManager_PushScenes(self, scenesTransitionSetupData, minDuration, afterMinDurationCallback, finishCallback);
    goodHitCounter = 0;
    badHitCounter = 0;
    notehit = 0;
    requestInProgress = false;
    PostRequest(false); //send a request to notify that the level has started
}


MAKE_AUTO_HOOK_MATCH(GameEnergyCounter_HandleNoteWasMissed, &GlobalNamespace::GameEnergyCounter::HandleNoteWasMissed, void, GlobalNamespace::GameEnergyCounter* self, GlobalNamespace::NoteController* noteController)
{
    //this method gets called when we miss any type of note, be it chains or full notes
    GameEnergyCounter_HandleNoteWasMissed(self, noteController);
    badHitCounter++;
}

MAKE_AUTO_HOOK_MATCH(ResultsViewController_SetDataToUI, &GlobalNamespace::ResultsViewController::SetDataToUI, void, GlobalNamespace::ResultsViewController* self)
{
    //this method gets called when the user goes to the results screen, failed or finished.
    ResultsViewController_SetDataToUI(self);
    PostRequest(false); //send a request to notify that the level has finished, just in case
}
