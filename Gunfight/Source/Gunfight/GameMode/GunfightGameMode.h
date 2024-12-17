// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GunfightGameMode.generated.h"

namespace MatchState
{
	extern GUNFIGHT_API const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AGunfightGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class AGunfightCharacter* ElimmedCharacter, class AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	void PlayerLeftGame(class AGunfightPlayerState* PlayerLeaving);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
private:
	float CountdownTime = 0.f;

	UPROPERTY()
	class AGunfightGameState* GunfightGameState;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
