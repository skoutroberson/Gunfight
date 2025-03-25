// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Gunfight/GunfightTypes/Team.h"
#include "GunfightPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	/**
	* Replication notifies
	*/

	virtual void OnRep_Score() override;
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

	// For the blueprint to set the PlayerState PlayerName to the oculus platform player name in OnRep_CharacterName().
	UFUNCTION(BlueprintCallable)
	void SetPlayerNameBP(const FString& NewName);

	/**
	* Team Swap
	*/

	void RequestTeamSwap();

	UFUNCTION(Server, Reliable)
	void ServerRequestTeamSwap();
	void ServerRequestTeamSwap_Implementation();

	void HandleTeamSwapRequest();

	UFUNCTION(Client, Reliable)
	void ClientTeamSwapped();
	void ClientTeamSwapped_Implementation();

	void SetTeamSwapRequested(bool bNewTeamSwapRequested);

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateGameInstanceTeamsMode();

	//UFUNCTION()
	//void OnRep_TeamSwapRequested();

	//UPROPERTY(ReplicatedUsing = OnRep_TeamSwapRequested, VisibleAnywhere)
	bool bTeamSwapRequested = false;

private:

	UPROPERTY()
	class AGunfightCharacter* Character;
	UPROPERTY()
	class AGunfightPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_Team, VisibleAnywhere)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();

public:
	FORCEINLINE int32 GetDefeats() const { return Defeats; }
	FORCEINLINE void SetDefeats(int32 NewDefeats) { Defeats = NewDefeats; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE bool HasRequestedTeamSwap() { return bTeamSwapRequested; }
	void SetTeam(ETeam TeamToSet);
};
