// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GunfightSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FGunSkinInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SKU;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPurchased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SkinName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTexture2D* SkinTexture;

	// only set by IAP Get Products by SKU in blueprints
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Price;
};

USTRUCT(BlueprintType)
struct FSkinInfoPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPurchased;

	// Index of the skin in the SkinsWidget GunSkins TArray
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Index;
};

/**
 * 
 */
UCLASS()
class GUNFIGHT_API UGunfightSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	
	// Preferred holster side for the player's gun
	UPROPERTY(VisibleAnywhere)
	bool bRightHolsterPreferred;
	
	// 0 is the default skin
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EquippedWeaponSkin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Kills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Deaths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Wins;

	/**
	* FString: SKU
	* 
	* bool: if the user has purchased the skin or not
	* int32: index of SkinInfo in the widget blueprint SkinInfos array
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FSkinInfoPair> GunSkinMap;

	// Returns the index of the skin; -1 if SKU was not found.
	UFUNCTION(BlueprintCallable)
	int32 UpdateSkinMapPurchase(FString SKU, bool bPurchased);

	/**
	* Settings Options
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TurnSensitivity = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlindersSensitivity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSnapTurning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxFPS = 90;

	//

	UPROPERTY(VisibleAnywhere)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere)
	uint32 UserIndex;

	UGunfightSaveGame();
};
