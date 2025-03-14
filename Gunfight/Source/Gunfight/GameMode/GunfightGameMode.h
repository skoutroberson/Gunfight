// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Gunfight/GunfightTypes/GunfightMatchState.h"
#include "GunfightGameMode.generated.h"

namespace MatchState
{
	extern GUNFIGHT_API const FName Warmup;
	extern GUNFIGHT_API const FName MatchInProgress;
	extern GUNFIGHT_API const FName MatchEnd;
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
	virtual void TickGunfightMatchState(float DeltaTime);
	virtual void PlayerEliminated(class AGunfightCharacter* ElimmedCharacter, class AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	void PlayerLeftGame(class AGunfightPlayerState* PlayerLeaving);

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	UPROPERTY(EditDefaultsOnly)
	float WaitingToStartTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightWarmupTime = 30.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightMatchTime = 300.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightCooldownTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightRoundStartTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightRoundTime = 105.f;

	UPROPERTY(EditDefaultsOnly)
	float GunfightRoundEndTime = 5.f;

	EGunfightMatchState GunfightMatchState;
	EGunfightRoundMatchState GunfightRoundMatchState;

	bool bTeamsMatch = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

	virtual void HandleGunfightWarmupStarted();
	virtual void HandleGunfightMatchStarted();
	virtual void HandleGunfightMatchEnded();

	void StartGunfightWarmup();
	void StartGunfightMatch();
	void EndGunfightMatch();
	void RestartGunfightMatch();

	void SetGunfightMatchState(EGunfightMatchState NewState);
	void OnGunfightMatchStateSet();
	
	float CountdownTime = 0.f;

private:

	UPROPERTY()
	class AGunfightGameState* GunfightGameState;

	// Chooses the farthest spawn away from other players.
	AActor* GetBestSpawnpoint();

	TArray<AActor*> Spawnpoints;
	TArray<AActor*> TeamASpawns;
	TArray<AActor*> TeamBSpawns;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	FORCEINLINE EGunfightMatchState GetGunfightMatchState() const { return GunfightMatchState; }
};
