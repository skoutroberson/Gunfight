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
protected:

private:
	float TopScore = 0.f;
	
	
};
