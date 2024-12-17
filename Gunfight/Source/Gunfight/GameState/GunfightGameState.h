// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
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

	// Keeps SortedPlayers sorted after a player scores, returns index for scoring player position in SortedPlayers after score.
	int32 PlayerScoreUpdate(AGunfightPlayerState* ScoringPlayer);

	int32 ScoringPlayerIndex = INDEX_NONE;

protected:

private:
	float TopScore = 0.f;

	UPROPERTY();
	class AGunfightPlayerController* LocalPlayerController;
	
};
