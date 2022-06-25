#include "UI/Settings/DataSenderFlowCoordinator.hpp"
#include "UI/Settings/DataSenderSettingsViewController.hpp"

#include "questui/shared/BeatSaberUI.hpp"
//#include "Utils/UIUtils.hpp"

#include "HMUI/TitleViewController.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

DEFINE_TYPE(DataSender::UI, DataSenderFlowCoordinator);

using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace QuestUI;
using namespace QuestUI::BeatSaberUI;

namespace DataSender::UI
{
	void DataSenderFlowCoordinator::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
	{
		if (firstActivation)
		{
			dataSenderSettingsViewController = reinterpret_cast<HMUI::ViewController*>(CreateViewController<DataSenderSettingsViewController*>());
			
			SetTitle(il2cpp_utils::newcsstr("Data Sender Settings"), ViewController::AnimationType::Out);
			showBackButton = true;

			ProvideInitialViewControllers(dataSenderSettingsViewController, nullptr, nullptr, nullptr, nullptr);
		}
	}

	void DataSenderFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController)
	{
		parentFlowCoordinator->DismissFlowCoordinator(this, ViewController::AnimationDirection::Horizontal, nullptr, false);
	}
}