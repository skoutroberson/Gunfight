// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gunfight/SaveGame/GunfightSaveGame.h"
#include "Kismet/GameplayStatics.h"

#define ECC_HandController ECollisionChannel::ECC_GameTraceChannel1
#define ECC_Weapon ECollisionChannel::ECC_GameTraceChannel2

UENUM(BlueprintType)
enum class ESide : uint8
{
	ES_None		UMETA(DisplayName = "None"),
	ES_Left		UMETA(DisplayName = "Left"),
	ES_Right	UMETA(DisplayName = "Right"),

	ES_MAX		UMETA(DisplayName = "DefaultMAX"),
};

using enum ESide;