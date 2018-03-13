#pragma once
#include "DeathmatchEvent.h"
#include "../../EventManager/EventManager/Public/EventManager.h"
#pragma comment(lib, "AAEventManager.lib")
#pragma comment(lib, "ArkApi.lib")
#include <fstream>
#include "json.hpp"

class DeathMatch : Event
{
private:
	int ArkShopPointsRewardMin, ArkShopPointsRewardMax, JoinMessages, JoinMessageDelaySeconds, PlayersNeededToStart;
	FString JoinEventCommand, ServerName;
public:
	virtual void InitConfig(const FString& JoinEventCommand, const FString& ServerName, const FString& Map)
	{
		this->JoinEventCommand = JoinEventCommand;
		this->ServerName = ServerName;
		if (!HasConfigLoaded())
		{
			const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DeathMatchEvent/" + Map.ToString() + ".json";

			std::ifstream file { config_path };

			if (!file.is_open()) throw std::runtime_error(fmt::format("Can't open {}.json", Map.ToString().c_str()).c_str());

			nlohmann::json config;

			file >> config;

			const std::string eName = config["Deathmatch"]["EventName"];
			const FString& EventName = ArkApi::Tools::Utf8Decode(eName).c_str();

			PlayersNeededToStart = config["Deathmatch"]["PlayersNeededToStart"];

			JoinMessages = config["Deathmatch"]["JoinMessages"];
			JoinMessageDelaySeconds = config["Deathmatch"]["JoinMessageDelaySeconds"];

			const bool StructureProtection = config["Deathmatch"]["StructureProtection"];
			const auto StructureProtectionPosition = config["Deathmatch"]["StructureProtectionPosition"];
			const int StructureProtectionDistacne = config["Deathmatch"]["StructureProtectionDistance"];

			ArkShopPointsRewardMin = config["Deathmatch"]["ArkShopPointsRewardMin"];
			ArkShopPointsRewardMax = config["Deathmatch"]["ArkShopPointsRewardMax"];
			if (ArkShopPointsRewardMin > ArkShopPointsRewardMax) ArkShopPointsRewardMax = ArkShopPointsRewardMin;
			if (ArkShopPointsRewardMin < 0) ArkShopPointsRewardMin = 0;

			InitDefaults(EventName, StructureProtection, FVector(StructureProtectionPosition[0], StructureProtectionPosition[1], StructureProtectionPosition[2]), StructureProtectionDistacne);

			const auto Spawns = config["Deathmatch"]["TeamASpawns"];
			for (const auto& Spawn : Spawns)
			{
				config = Spawn["Position"];
				AddSpawn(FVector(config[0], config[1], config[2]));
			}
			file.close();
		}
		Init();
	}

	virtual void Update()
	{
		Log::GetLog()->warn("Update() State({})", (int)GetState());
		switch (GetState())
		{
		case EventState::WaitingForPlayers:
			if (WaitForPassed())
			{
				if (IncreaseCounter() == JoinMessages)
				{
					if (EventManager::GetEventManager()->GetEventPlayerCount() < 2)
					{
						ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, L"[Event] {} Failed to start needed 2 Players", *GetName());
						SetState(EventState::Finnished);
					}
					else SetState(EventState::TeleportingPlayers);
				} else ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, L"[Event] {} Starting in {} Seconds, To Join Type {]", *GetName(), ((JoinMessages - GetCounter()) * JoinMessageDelaySeconds), *JoinEventCommand);
				WaitFor(JoinMessageDelaySeconds);
			}
			break;
		case EventState::TeleportingPlayers:
			EventManager::TeleportEventPlayers(false, true, true, GetSpawns());
			EventManager::GetEventManager()->SendChatMessageToAllEventPlayers(ServerName, L"[Event] {} Starting in 30 Seconds", *GetName());
			WaitFor(30);
			SetState(EventState::WaitForFight);
			break;
		case EventState::WaitForFight:
			if (WaitForPassed())
			{
				EventManager::GetEventManager()->SendChatMessageToAllEventPlayers(ServerName, L"[Event] {} Started Kill or Be Killed!", *GetName());
				SetState(EventState::Fighting);
			}
			break;
		case EventState::Fighting:
			break;
		case EventState::Rewarding:
			break;
		}
	}
};

DeathMatch* DmEvent;

void InitEvent()
{
	Log::Get().Init("Deathmatch Event");
	DmEvent = new DeathMatch();
	EventManager::AddEvent((Event*)DmEvent);
}

void RemoveEvent()
{
	EventManager::RemoveEvent((Event*)DmEvent);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitEvent();
		break;
	case DLL_PROCESS_DETACH:
		RemoveEvent();
		break;
	}
	return TRUE;
}