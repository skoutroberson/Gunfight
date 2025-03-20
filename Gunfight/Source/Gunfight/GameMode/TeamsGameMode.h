// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GunfightGameMode.h"
#include "Gunfight/GunfightTypes/Team.h"
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
	virtual void PlayerEliminated(class AGunfightCharacter* ElimmedCharacter, class AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController);

protected:
	virtual void BeginPlay() override;

	virtual void HandleMatchHasStarted() override;

	virtual void SetGunfightRoundMatchState(EGunfightRoundMatchState NewRoundMatchState) override;
	void OnGunfightRoundMatchStateSet();

	void StartGunfightRoundMatch();
	void StartGunfightRound();
	void EndGunfightRound();
	void RestartGunfightRound();
	void RestartGunfightRoundMatch();
	void EndGunfightRoundMatch();

private:

	bool ShouldEndRound(ETeam TeamToCheck);
	bool AreAllPlayersDead(ETeam TeamToCheck);
	void UpdateTeamScore(ETeam LosingTeam);
	bool ShouldEndGame();
	ETeam GetWinningTeam();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 WinningTeamScore = 7;

public:
	
};
