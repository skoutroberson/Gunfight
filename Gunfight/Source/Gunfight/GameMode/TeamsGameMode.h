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

	virtual void TickGunfightMatchState(float DeltaTime) override;

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;

protected:
	virtual void HandleMatchHasStarted() override;

	void StartGunfightRoundMatch();
	void SetGunfightRoundMatchState(EGunfightRoundMatchState NewRoundMatchState);
	void OnGunfightRoundMatchStateSet();

private:

public:
	
};
