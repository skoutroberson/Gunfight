// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"

void UGunfightGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bool bSuccess = true;

	GunfightSaveGame = Cast<UGunfightSaveGame>(UGameplayStatics::LoadGameFromSlot(FString("GunfightSaveSlot"), 0));

	if (GunfightSaveGame)
	{
		Kills = GunfightSaveGame->Kills;
		Deaths = GunfightSaveGame->Deaths;
		Wins = GunfightSaveGame->Wins;
	}
	else
	{
		GunfightSaveGame = Cast<UGunfightSaveGame>(UGameplayStatics::CreateSaveGameObject(UGunfightSaveGame::StaticClass()));
		if (GunfightSaveGame)
		{
			// Save the data immediately.
			if (UGameplayStatics::SaveGameToSlot(GunfightSaveGame, FString("GunfightSaveSlot"), 0))
			{
				// Save succeeded.
			}
			else
			{
				bSuccess = false;
			}
		}
	}

	NotifyGameInstanceThatGunfightInitialized(bSuccess);
}

void UGunfightGameInstanceSubsystem::Deinitialize()
{
	if (GunfightSaveGame)
	{
		UGameplayStatics::SaveGameToSlot(GunfightSaveGame, FString("GunfightSaveSlot"), 0);
	}
}

void UGunfightGameInstanceSubsystem::AddToKills()
{
	++Kills;
	if (GunfightSaveGame)
	{
		++GunfightSaveGame->Kills;
		UGameplayStatics::AsyncSaveGameToSlot(GunfightSaveGame, GunfightSaveGame->SaveSlotName, 0);
	}
}

void UGunfightGameInstanceSubsystem::AddToDefeats()
{
	++Deaths;
	if (GunfightSaveGame)
	{
		++GunfightSaveGame->Deaths;
		UGameplayStatics::AsyncSaveGameToSlot(GunfightSaveGame, GunfightSaveGame->SaveSlotName, 0);
	}
}

void UGunfightGameInstanceSubsystem::AddToWins()
{
	++Wins;
	if (GunfightSaveGame)
	{
		++GunfightSaveGame->Wins;
		UGameplayStatics::AsyncSaveGameToSlot(GunfightSaveGame, GunfightSaveGame->SaveSlotName, 0);
	}
}

void UGunfightGameInstanceSubsystem::NotifyGameInstanceThatGunfightInitialized(bool bGunfightInit) const
{
	FOutputDeviceNull OutputDeviceNull;
	GetGameInstance()->CallFunctionByNameWithArguments(
		*FString::Printf(TEXT("GunfightInitialized %s"), bGunfightInit ? TEXT("true") : TEXT("false")),
		OutputDeviceNull,
		nullptr,
		true);
}
