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
	AGunfightPlayerController();
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDScoreboard();
	void SetHUDPing();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); // Synced with server world clock
	virtual void ReceivedPlayer() override; // Sync with server clock as soon as possible

	virtual void PawnLeavingGame();

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

	UFUNCTION(Client, Reliable)
	void ClientPostLoginSetMatchState(EGunfightMatchState NewState);
	void ClientPostLoginSetMatchState_Implementation(EGunfightMatchState NewState);

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);
	void ClientJoinMidGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);

	UFUNCTION(Client, Reliable)
	void ClientRestartGame(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);
	void ClientRestartGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime);

	// gunfight match state is not getting replicated after the first match for some reason so I'm using this client RPC to correctly replicate match state... wtf
	UFUNCTION(Client, Reliable)
	void ClientUpdateMatchState(EGunfightMatchState StateOfMatch);
	void ClientUpdateMatchState_Implementation(EGunfightMatchState StateOfMatch);

	UPROPERTY(ReplicatedUsing = OnRep_GunfightMatchState, VisibleAnywhere)
	EGunfightMatchState GunfightMatchState = EGunfightMatchState::EGMS_Uninitialized;

	/**
	* Matchmaking
	* 
	* This works with the GameInstance and BP_GunfightPlayerController.
	* 
	* This is so the clients can reconnect to the new public/private match session once they disconnect from the host. 
	* 
	* When the Listen Server host decides to "Host" a new match or join a QuickMatch, if other clients are in the same session as them: 
	* 
	* Each client saves the lobby/match session info, sets the game instance variable: bJoiningHost to true, 
	* and confirms with the Host, Once all of the clients have confirmed:
	* 
	* The host destroys the current session and creates the new session with the match id, or joins the QuickMatch session id.
	* 
	* Each Client disconnects from the host and because bJoiningSession is true, OnDisconnectError and OnRoomFound(false) will try to NetworkTravel to
	* the new match session. 
	*
	* A client will try to connect to the new session MaxHostJoinTries times.
	* 
	* TODO: Make functionality for returning back to the lobby if the host quits out of the match (Low priority because even CS2 doesn't have this).
	* TODO: Put this shit in a component UMatchmakingComponent
	*/

	UFUNCTION(BlueprintCallable)
	void HostMatchGo(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void AllClientsReady();

	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientGiveHostMatchDetails(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName);
	void ClientGiveHostMatchDetails_Implementation(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName);

	UFUNCTION(Server, Reliable)
	void ServerReceivedHostMatchDetails();
	void ServerReceivedHostMatchDetails_Implementation();

	// Once all clients confirm they received the new match details, the server will create and travel to the new match session, and the clients will travel to it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite);
	int32 ClientsReceivedMatchDetails = 0;

	// Called by the clients so after they disconnect from the host, they still have Lobby / Match info
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateGameInstanceMatchDetails(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName);

	// Quick match

	// Returns the number of players in the lobby
	UFUNCTION(BlueprintCallable)
	int32 GetLobbySize();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWon = false;
	
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

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	void ShowReturnToMainMenu();

	UFUNCTION()
	void OnRep_GunfightMatchState();

	virtual void HandleGunfightWarmupStarted();
	virtual void HandleGunfightMatchStarted();
	virtual void HandleGunfightCooldownStarted();

	void UpdateSaveGameData(bool bWon);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateLeaderboardKills();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateLeaderboardWins();

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

	UPROPERTY()
	class UStereoLayerComponent* StereoLayer;

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
