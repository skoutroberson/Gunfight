// Fill out your copyright notice in the Description page of Project Settings.

#include "GunfightSaveGame.h"
#include "Gunfight/Gunfight.h"

int32 UGunfightSaveGame::UpdateSkinMapPurchase(FString SKU, bool bPurchased)
{
	int32 Index = -1;

	FSkinInfoPair* FoundPair = GunSkinMap.Find(SKU);
	if (FoundPair)
	{
		FoundPair->bPurchased = bPurchased;
		Index = FoundPair->Index;
	}

	return Index;
}

UGunfightSaveGame::UGunfightSaveGame()
{
	SaveSlotName = TEXT("GunfightSaveSlot");
	UserIndex = 0;

	bRightHolsterPreferred = true;
	EquippedWeaponSkin = 0;
	Kills = 0;
	Deaths = 0;
	Wins = 0;

	TurnSensitivity = 5.f;
	BlindersSensitivity = 0.f;
	bSnapTurning = false;
	MaxFPS = 90;

	GunSkinMap.Add(FString("Default"), {true, 0});
	GunSkinMap.Add(FString("GOLD-1911"), {false, 1});
	//GunSkinMap.Add(FString("FreeTest"), { true, 2 });
	GunSkinMap.Add(FString("BLACK-1911"), { false, 2 });
}