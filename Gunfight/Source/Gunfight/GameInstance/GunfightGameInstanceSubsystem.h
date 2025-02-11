// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gunfight/SaveGame/GunfightSaveGame.h"
#include "GunfightGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API UGunfightGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// End USubsystem
	virtual void Deinitialize() override;

	/**
	* Save Game
	*/

	int32 Kills;
	int32 Deaths;
	int32 Wins;

	UPROPERTY(EditAnywhere, BlueprintReadWrite);
	UGunfightSaveGame* GunfightSaveGame;

	void AddToKills();
	void AddToDefeats();
	void AddToWins();

protected:
	void NotifyGameInstanceThatGunfightInitialized(bool bGunfightInit) const;

};
