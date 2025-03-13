// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GunfightGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API ATeamsGameMode : public AGunfightGameMode
{
	GENERATED_BODY()

public:
	
	ATeamsGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	virtual void HandleMatchHasStarted() override;
	
};
