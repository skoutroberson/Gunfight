// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gunfight/SaveGame/GunfightSaveGame.h"
#include "Kismet/GameplayStatics.h"

#define ECC_HandController ECollisionChannel::ECC_GameTraceChannel1
#define ECC_Weapon ECollisionChannel::ECC_GameTraceChannel2

#define EST_Concrete EPhysicalSurface::SurfaceType4
#define EST_Metal EPhysicalSurface::SurfaceType5
#define EST_Tile EPhysicalSurface::SurfaceType6
#define EST_Wood EPhysicalSurface::SurfaceType7

UENUM(BlueprintType)
enum class ESide : uint8
{
	ES_None		UMETA(DisplayName = "None"),
	ES_Left		UMETA(DisplayName = "Left"),
	ES_Right	UMETA(DisplayName = "Right"),
};

using enum ESide;