// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Gunfight/GunfightTypes/GunfightMatchState.h"
#include "Gunfight/GunfightTypes/Team.h"
#include "GunfightGameState.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class AGunfightPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<AGunfightPlayerState*> TopScoringPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_SortedPlayers)
	TArray<AGunfightPlayerState*> SortedPlayers;

	UFUNCTION()
	void OnRep_SortedPlayers();

	int32 CurrentPlayerCount = 0;

	/**
	* Updates SortedPlayers on score, keeps it sorted.
	* 
	* returns index of scoring player position in SortedPlayers after score.
	*/
	int32 HandlePlayerScore(AGunfightPlayerState* ScoringPlayer);

	int32 ScoringPlayerIndex = INDEX_NONE;

	/**
	* Teams
	*/

	void RedTeamScores();
	void BlueTeamScores();

	TArray<AGunfightPlayerState*> RedTeam;
	TArray<AGunfightPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;

	UFUNCTION()
	void OnRep_RedTeamScore();

	UFUNCTION()
	void OnRep_BlueTeamScore();

	/** Gunfight match state has changed */
	UFUNCTION()
	void OnRep_GunfightMatchState();

	/** Updates the gunfight match state and calls the appropriate transition functions, only valid on server */
	void SetGunfightMatchState(EGunfightMatchState NewState);

	/**
	* Team Swap
	*/
	TQueue<AGunfightPlayerState*> RedTeamSwappers;
	TQueue<AGunfightPlayerState*> BlueTeamSwappers;

	void QueueOrSwapSwapper(AGunfightPlayerState* TeamSwapper);
	void SwapTeams(AGunfightPlayerState* RedSwapper, AGunfightPlayerState* BlueSwapper);

	bool IsMatchEnding();
protected:

	UPROPERTY(ReplicatedUsing = OnRep_GunfightMatchState)
	EGunfightMatchState GunfightMatchState;

	void HandleGunfightWarmupStarted();
	void HandleGunfightMatchStarted();
	void HandleGunfightCooldownStarted();

private:
	float TopScore = 0.f;

	UPROPERTY();
	class AGunfightPlayerController* LocalPlayerController;

	void UpdateLocalHUDTeamScore(float ScoreAmount, ETeam TeamToUpdate);

	int32 WinningScore = 5;

public:
	ETeam GetWinningTeam();
	
};
