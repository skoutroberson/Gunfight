// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRPlayerController.h"
#include "Gunfight/GunfightTypes/ScoreboardUpdate.h"
#include "Gunfight/GunfightTypes/GunfightMatchState.h"
#include "GunfightPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightPlayerController : public AVRPlayerController
{
	GENERATED_BODY()
	
public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDScoreboard();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // Synced with server world clock
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible

	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();
	void HandleCooldown2();

	void SetAnnouncementVisibility(bool bVisible);

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	void InitializeHUD();

	void UpdateScoreboard(class AGunfightPlayerState* PlayerToUpdate, EScoreboardUpdate Type);

	void SetScoreboardVisibility(bool bVisible);

	void SetGunfightMatchState(EGunfightMatchState NewState);
	
protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();

	/**
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	void ServerRequestServerTime_Implementation(float TimeOfClientRequest);

	// Reports the current server time to the client in reponse to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	void ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	void ServerCheckMatchState_Implementation();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);
	void ClientJoinMidGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();

	UPROPERTY(ReplicatedUsing = OnRep_GunfightMatchState)
	EGunfightMatchState GunfightMatchState;

	UFUNCTION()
	void OnRep_GunfightMatchState();

	virtual void HandleGunfightWarmupStarted();
	virtual void HandleGunfightMatchStarted();
	virtual void HandleGunfightCooldownStarted();

private:
	UPROPERTY()
	class AGunfightHUD* GunfightHUD;

	UPROPERTY()
	class AGunfightGameMode* GunfightGameMode;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WaitingToStartTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;
	float WarmupTime = 0.f;
	float MatchEndTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	bool bInitializeCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	float HighPingRunningTime = 0.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 10.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);
	void ServerReportPingStatus_Implementation(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 120.f;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* HealthColorCurve;

	public:
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() { return CharacterOverlay; }

	// fills PlayerSorted with Players, sorted by score
	void SortPlayersByScore(TArray<TObjectPtr<APlayerState>>& Players);

	// inserts player into PlayersSorted using binary search
	void BinaryInsertPlayer(TObjectPtr<APlayerState>& ThisPlayer, TArray<TObjectPtr<APlayerState>>& Players);

	// players sorted by score
	TArray<TObjectPtr<APlayerState>> PlayersSorted;

	void SetHUDScoreboardScores(int32 StartIndex, int32 EndIndex);

	UPROPERTY()
	class AGunfightGameState* GunfightGameState;

	void DrawSortedPlayers();

	// for intializing the scoreboard
	bool bPlayerStateInitialized = false;
	
public:
	AGunfightHUD* GetGunfightHUD();
};
