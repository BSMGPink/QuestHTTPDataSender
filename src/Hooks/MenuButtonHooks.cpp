#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "hooks.hpp"
#include "config.hpp"
#include "assets.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "System/Action.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/CustomTypes/Components/ExternalComponents.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "UnityEngine/UI/Button.hpp"
#include "HMUI/ButtonSpriteSwap.hpp"

#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Events/UnityAction_1.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "Utils/WebUtils.hpp"

#define MakeDelegate(DelegateType, varName) (il2cpp_utils::MakeDelegate<DelegateType>(classof(DelegateType), varName))

bool setIcons = false;

MAKE_AUTO_HOOK_MATCH(MainMenuViewController_DidActivate, &GlobalNamespace::MainMenuViewController::DidActivate, void, GlobalNamespace::MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    //this method gets called as soon as the main menu gets activated
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling); //run original method
    if(setIcons) return; //stops us from running below twice, can crash upon level completion 

    //reusing the editor button
    self->beatmapEditorButton->get_gameObject()->set_active(true);
    self->beatmapEditorButton->get_transform()->GetComponent<HMUI::HoverHint*>()->set_text("Edit GamerTag"); //rename the hover text

    //swap the default button icons to our own button images
    HMUI::ButtonSpriteSwap* spriteSwap = self->beatmapEditorButton->get_gameObject()->GetComponent<HMUI::ButtonSpriteSwap*>();
    auto highlightedImage = QuestUI::BeatSaberUI::VectorToSprite(std::vector<uint8_t>(_binary_IconSelected_png_start,_binary_IconSelected_png_end));
    auto defaultImage = QuestUI::BeatSaberUI::VectorToSprite(std::vector<uint8_t>(_binary_Icon_png_start,_binary_Icon_png_end));
    spriteSwap->normalStateSprite = defaultImage;
    spriteSwap->highlightStateSprite = highlightedImage;
    spriteSwap->pressedStateSprite = highlightedImage;
    spriteSwap->disabledStateSprite = defaultImage;
    setIcons = true;

}


std::string usernameToSet = "";
TMPro::TextMeshProUGUI* infoText;
HMUI::ModalView* usernameModal;
HMUI::ModalView* warningModal;
GlobalNamespace::MainMenuViewController* menuButtonSelf;
HMUI::InputFieldView* gamerTagTextBox;
UnityEngine::UI::Button* setGamerTagButton;
UnityEngine::UI::Button* closeGamerTagButton;

int connectionAttempts = 1;
void GamerTagIsMatch(){
    WebUtils::GetAsync(config.gamerTagSearchURL + usernameToSet, 3L, [&](long code, std::string result){
        switch (code)
        {
            //Error code 500 is if its invalid, so I dont need to check the json itself
            case 200:
                QuestUI::MainThreadScheduler::Schedule([] { //run on the next frame
                    config.gamerTag = usernameToSet; //set the username if its a match
                    SaveConfig();
                    usernameModal->Hide(true, nullptr); //close the menu
                    infoText->SetText("Enter Your GamerTag"); //refresh the text
                });
                break;
            case 500:
                QuestUI::MainThreadScheduler::Schedule([] { //run on the next frame
                    infoText->SetText("Enter Your GamerTag<br><size=4><color=red>GamerTag " + usernameToSet + " not found</color></size>"); //set to failure text
                });
                break;
            default:
                if(connectionAttempts < 4){ //if there was a connection error, and there have been less than 4 connection errors
                    
                    QuestUI::MainThreadScheduler::Schedule([] { //run on the next frame
                        infoText->SetText("Enter Your GamerTag<br><size=4>Checking GamerTag... (" + std::to_string(connectionAttempts) + "/3)</size>"); //set to reattempting connection text
                        connectionAttempts++;
                        GamerTagIsMatch(); //run method again to reattempt the connection
                    });
                }else{ //if we've attempted to send the request 4 times, then show an internet connection warning
                    QuestUI::MainThreadScheduler::Schedule([] { //run on the next frame
                        infoText->SetText("Enter Your GamerTag<br><size=4><color=red>Internet Connection Error</color></size>"); //set to internet connection error text
                    });
                }
                break;
        }
        QuestUI::MainThreadScheduler::Schedule([] { //run on the next frame
            setGamerTagButton->set_interactable(true); //allow the buttons to be interacted with again
            closeGamerTagButton->set_interactable(true);
        });
   });
}


MAKE_AUTO_HOOK_MATCH(MainMenuViewController_HandleMenuButton, &GlobalNamespace::MainMenuViewController::HandleMenuButton, void, GlobalNamespace::MainMenuViewController* self, GlobalNamespace::MainMenuViewController::MenuButton menuButton)
{
//this method gets called as soon as a button within the main menu is pressed.
    if(menuButton == GlobalNamespace::MainMenuViewController::MenuButton::BeatmapEditor){
        if(usernameModal == nullptr){
            //if the menu doesnt exist yet
            usernameModal = QuestUI::BeatSaberUI::CreateModal(self->soloButton->get_transform()->get_parent(), UnityEngine::Vector2(110, 60), UnityEngine::Vector2(0, 0), [](HMUI::ModalView *modal){ }, false); //create it
            auto* horizontalLayoutGroup = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(usernameModal);
            auto* verticalLayoutGroup = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(horizontalLayoutGroup);//create a vertical and horizontal layout group, which allows for positioning of elements
            verticalLayoutGroup->set_spacing(5);

            horizontalLayoutGroup->set_spacing(40);
            infoText = QuestUI::BeatSaberUI::CreateText(verticalLayoutGroup->get_transform(), "Enter Your GamerTag"); //create the info text
            infoText->set_alignment(TMPro::TextAlignmentOptions::Center);
            infoText->set_fontSize(9);
            infoText->set_fontStyle(TMPro::FontStyles::Italic); //sets the text to italics, normal will have graphical issues upon opening the first time as it will be italic and then pop to normal.
            gamerTagTextBox = QuestUI::BeatSaberUI::CreateStringSetting(verticalLayoutGroup->get_transform(), "GamerTag", config.gamerTag, {0,0}, {0,0,0}, [](std::string value) {
                usernameToSet = value;
            });
            usernameToSet = config.gamerTag; //make the initial username the already saved username

            auto* horizontalButtonLayoutGroup = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());

            horizontalButtonLayoutGroup->set_spacing(10);
            horizontalButtonLayoutGroup->set_padding(UnityEngine::RectOffset::New_ctor(15, 15, 0, 0));
            closeGamerTagButton = QuestUI::BeatSaberUI::CreateUIButton(horizontalButtonLayoutGroup->get_transform(), "Close", []{ 
                usernameModal->Hide(true, nullptr);
            }); //create close button
            
            setGamerTagButton = QuestUI::BeatSaberUI::CreateUIButton(horizontalButtonLayoutGroup->get_transform(), "Set", []{ 
                
                if(config.gamerTagCheckRequirement){
                    setGamerTagButton->set_interactable(false);
                    closeGamerTagButton->set_interactable(false);
                    infoText->SetText("Enter Your GamerTag<br><size=4>Checking GamerTag...</size>");
                    connectionAttempts = 1;
                    GamerTagIsMatch();
                }else{
                    config.gamerTag = usernameToSet;
                    SaveConfig();
                    usernameModal->Hide(true, nullptr);
                    infoText->SetText("Enter Your GamerTag");
                }
            }); //create set button
        }

        usernameModal->Show(true,true, MakeDelegate(System::Action*, (std::function<void()>)[](){ 
            gamerTagTextBox->SetText(config.gamerTag);  //this fixes a visual issue where the text blurs upon opening
            infoText->SetText("<size=9>Enter Your GamerTag</size>");

        }));



    }else if(menuButton == GlobalNamespace::MainMenuViewController::MenuButton::SoloFreePlay && config.gamerTag == ""){
        //if the solo button is pressed
        menuButtonSelf = self;
        if(warningModal == nullptr){ //if the warning menu hasnt been made yet
            warningModal = QuestUI::BeatSaberUI::CreateModal(self->soloButton->get_transform()->get_parent(), UnityEngine::Vector2(90, 50), UnityEngine::Vector2(0, 0), nullptr); //create the warning menu
            auto* horizontalLayoutGroup = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(warningModal);
            auto* verticalLayoutGroup = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(horizontalLayoutGroup);
            auto* infoText = QuestUI::BeatSaberUI::CreateText(verticalLayoutGroup->get_transform(), "<color=red><size=8>WARNING</size></color><br>No GamerTag Is Currently Set<br>Do You Want To Continue?");
            infoText->set_alignment(TMPro::TextAlignmentOptions::Center);
            verticalLayoutGroup->set_spacing(5); //create warning text
            auto* horizontalButtonLayoutGroup = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(verticalLayoutGroup->get_transform());

            horizontalButtonLayoutGroup->set_spacing(10.0);
            QuestUI::BeatSaberUI::CreateUIButton(horizontalButtonLayoutGroup->get_transform(), "Close", []{ 
                warningModal->Hide(true, nullptr); //close the menu
            }); //create close button
            
            QuestUI::BeatSaberUI::CreateUIButton(horizontalButtonLayoutGroup->get_transform(), "Continue", []{ 
                warningModal->Hide(true, nullptr); //close the menu
                MainMenuViewController_HandleMenuButton(menuButtonSelf, GlobalNamespace::MainMenuViewController::MenuButton::SoloFreePlay); //open the menu
            });
        }
        warningModal->Show(true,true,nullptr); //open the menu
        
    }else{
	    MainMenuViewController_HandleMenuButton(self, menuButton); //run the original method
    }


}

MAKE_AUTO_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<::Zenject::DiContainer*>*  finishCallback)
{
    //selecting an option in the base game settings restarts the game entirely, running the garbage collector and destroying our objects above, but not nulling them.
    //This means we have to null the objects we made above ourselves again so we can remake the menu 
    MenuTransitionsHelper_RestartGame(self, finishCallback);
    usernameModal = nullptr; //set the menu modal to null so it can be remade above.
    setIcons = false;
}