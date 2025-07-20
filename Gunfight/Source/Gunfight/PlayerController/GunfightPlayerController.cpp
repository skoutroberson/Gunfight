// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightPlayerController.h"
#include "Gunfight/HUD/GunfightHUD.h"
#include "Gunfight/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/GameMode/GunfightGameMode.h"
#include "Gunfight/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Components/Image.h"
#include "GameFramework/PlayerState.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "Components/GridPanel.h"
#include "Components/Widget.h"
#include "Components/StereoLayerComponent.h"
#include "Gunfight/GameInstance/GunfightGameInstanceSubsystem.h"
#include "Sound/SoundCue.h"

AGunfightPlayerController::AGunfightPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AGunfightPlayerController::BeginPlay()
{
	Super::BeginPlay();

	GunfightHUD = Cast<AGunfightHUD>(GetHUD());
	ServerCheckMatchState();
}

void AGunfightPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightPlayerController, MatchState);
	DOREPLIFETIME(AGunfightPlayerController, GunfightMatchState);
	DOREPLIFETIME(AGunfightPlayerController, GunfightRoundMatchState);
	DOREPLIFETIME(AGunfightPlayerController, LevelStartingTime);
}

void AGunfightPlayerController::HostMatchGo(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName)
{
	if (GetWorld() == nullptr) return;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer && !GunfightPlayer->IsLocalController())
		{
			GunfightPlayer->ClientGiveHostMatchDetails(HostMatchId, HostLobbyId, HostDestinationApiName, HostLevelName);
		}
	}
}

void AGunfightPlayerController::ClientGiveHostMatchDetails_Implementation(const FString& HostMatchId, const FString& HostLobbyId, const FString& HostDestinationApiName, const FString& HostLevelName)
{
	UpdateGameInstanceMatchDetails(HostMatchId, HostLobbyId, HostDestinationApiName, HostLevelName);
	ServerReceivedHostMatchDetails();

	AGunfightCharacter* GC = Cast<AGunfightCharacter>(GetPawn());
	if(GC)
	{
		GC->DebugLogMessage(FString("Client Give Host Match Details."));
	}
}

void AGunfightPlayerController::ServerReceivedHostMatchDetails_Implementation()
{
	++ClientsReceivedMatchDetails;

	AGunfightCharacter* GC = Cast<AGunfightCharacter>(GetPawn());
	if (GC)
	{
		GC->DebugLogMessage(FString("Server Receive Host Match Details."));
	}

	int32 PlayersInGame = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		++PlayersInGame;
	}
	
	if (ClientsReceivedMatchDetails >= PlayersInGame - 1)
	{
		AllClientsReady();
		ClientsReceivedMatchDetails = 0;
	}
	
}

int32 AGunfightPlayerController::GetLobbySize()
{
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			++PlayerCount;
		}
	}
	return PlayerCount;
}

void AGunfightPlayerController::UpdateTeamSwapText(const FString& NewSwapText)
{
	if (CharacterOverlay == nullptr || CharacterOverlay->SwapText == nullptr) return;
	CharacterOverlay->SwapText->SetRenderOpacity(1.f);
	CharacterOverlay->SwapText->SetText(FText::FromString(NewSwapText));
}

void AGunfightPlayerController::UpdateAnnouncement(bool bShow, const FSlateColor& Color, const FString& NewString)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr || GunfightHUD->CharacterOverlay == nullptr) return;
	UCharacterOverlay* CharOverlay = GunfightHUD->CharacterOverlay;

	bool bHUDValid = CharOverlay &&
		CharOverlay->AnnouncementText &&
		CharOverlay->AnnouncementBackground;

	if (!bHUDValid) return;

	CharOverlay->AnnouncementText->SetRenderOpacity(bShow);
	CharOverlay->AnnouncementBackground->SetRenderOpacity(bShow);

	CharOverlay->AnnouncementText->SetColorAndOpacity(Color);
	CharOverlay->AnnouncementText->SetText(FText::FromString(NewString));
}

void AGunfightPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

}

void AGunfightPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);

	//DrawSortedPlayers();
}

void AGunfightPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		GunfightHUD = GetHUD<AGunfightHUD>();
		//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, 14.f, FColor::Red, FString("Poll: 1"));
		if (GunfightHUD && GunfightHUD->CharacterOverlay)
		{
			//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, 14.f, FColor::Red, FString("Poll: 2"));
			CharacterOverlay = GunfightHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, 14.f, FColor::Red, FString("Poll: 3"));
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
				UpdateScoreboard(GetPlayerState<AGunfightPlayerState>(), EScoreboardUpdate::ESU_MAX);

				AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
				//if (GunfightCharacter && GunfightCharacter->GetDefaultWeapon())
				if (GunfightCharacter && GunfightCharacter->GetDefaultWeapon())
				{
					//GunfightCharacter->GetDefaultWeapon()->SetHUDAmmo();

					if (GunfightCharacter->VRStereoLayer)
					{
						StereoLayer = GunfightCharacter->VRStereoLayer;
					}
				}

				//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, FString("AGunfightPlayerController::PollInit() This better not be ticking..."));
				// TODO: CHANGE THIS DOWN HERE 4/5/2025
				if (bInitializeCarriedAmmo)
				{
					SetHUDCarriedAmmo(HUDCarriedAmmo, false);
					SetHUDCarriedAmmo(HUDCarriedAmmo, true);
				}
				if (bInitializeWeaponAmmo)
				{
					SetHUDWeaponAmmo(HUDWeaponAmmo, false);
					SetHUDWeaponAmmo(HUDWeaponAmmo, true);
				}
			}
		}
		else if(GunfightHUD)
		{
			AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
			if (GunfightCharacter)
			{
				GunfightHUD->CharacterOverlay = Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject());
			}
		}
	}
	else if (!CharacterOverlay->bIsConstructed)
	{
		//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds * 1.1f, FColor::Red, FString("Poll: 4"));
		CharacterOverlay->FillPlayers();
	}
	else if (!bPlayerStateInitialized && GetPlayerState<AGunfightPlayerState>())
	{
		//if (GEngine && IsLocalController())GEngine->AddOnScreenDebugMessage(-1, 14.f, FColor::Red, FString("Poll: 5"));
		bPlayerStateInitialized = true;
		UpdateScoreboard(GetPlayerState<AGunfightPlayerState>(), EScoreboardUpdate::ESU_MAX);
		//GetPlayerState<AGunfightPlayerState>()->
	}
}

void AGunfightPlayerController::InitializeHUD()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter)
	{
		GunfightCharacter->DebugLogMessage(FString("InitializeHUD"));
	}

	GunfightHUD = GunfightHUD == nullptr ? GetHUD<AGunfightHUD>() : GunfightHUD;

	if (GunfightCharacter && GunfightHUD && GunfightCharacter->CharacterOverlayWidget && GunfightCharacter->VRStereoLayer)
	{
		CharacterOverlay = Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject());
		if (CharacterOverlay)
		{
			GunfightHUD->CharacterOverlay = CharacterOverlay;
			GunfightHUD->StereoLayer = GunfightCharacter->VRStereoLayer;
			StereoLayer = GunfightCharacter->VRStereoLayer;

			if (GunfightRoundMatchState != EGunfightRoundMatchState::EGRMS_Uninitialized)
			{
				if (GEngine && IsLocalController()) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, FString::Printf(TEXT("How is this ticking: %s"), *GetName()));
				OnRep_GunfightRoundMatchState();
				SetHUDScoreboardTeamScores();
			}
			else
			{
				if (GEngine && IsLocalController()) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, FString::Printf(TEXT("PLEASE DONT BE TICKING: %s"), *GetName()));
				OnRep_GunfightMatchState();
			}
			

			if (GetWorld() && GetWorld()->GetMapName().Contains(FString("Hideout")))
			{
				UpdateHUD(false, false, false, false, false);
				/*
				if (CharacterOverlay->AnnouncementText && CharacterOverlay->AnnouncementBackground)
				{
					CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
					CharacterOverlay->AnnouncementBackground->SetRenderOpacity(0.f);
					
				}*/
			}

			/*
			// I Will need to check other gamemodes if I don't derive the next gamemodes from AGunfightGameMode
			AGunfightGameMode* GM = Cast<AGunfightGameMode>(UGameplayStatics::GetGameMode(this));
			if (GM == nullptr)
			{
				CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
			}
			*/
			
			//SetHUDScoreboardScores(0, 9); // hardcoded 10 players
		}
	}
}

// this function sucks TODO: burn it and make another
void AGunfightPlayerController::UpdateScoreboard(AGunfightPlayerState* PlayerToUpdate, EScoreboardUpdate Type)
{
	if (IsLocalController() == false) return;

	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());

	if (GunfightCharacter == nullptr || GunfightGameState == nullptr) return;
	if (PlayerToUpdate == nullptr && Type < EScoreboardUpdate::ESU_Join) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr || StereoLayer == nullptr) return;
	if (CharacterOverlay->ScoreboardInfos.IsEmpty()) return;

	if (Type == EScoreboardUpdate::ESU_Score) // player gets a kill
	{
		int32 IndexToUpdate = GunfightGameState->HandlePlayerScore(PlayerToUpdate);

		if (AreWeInATeamsMatch())
		{
			SetHUDScoreboardTeamScores();
		}
		else
		{
			if (GunfightGameState->ScoringPlayerIndex != INDEX_NONE)
			{
				SetHUDScoreboardScores(IndexToUpdate, GunfightGameState->ScoringPlayerIndex + 1);
			}
		}
	}
	else if (Type == EScoreboardUpdate::ESU_Death) // player dies
	{
		int32 IndexToUpdate = GunfightGameState->SortedPlayers.Find(PlayerToUpdate);

		if (AreWeInATeamsMatch()) SetHUDScoreboardTeamScores();
		else CharacterOverlay->ScoreUpdate(IndexToUpdate, PlayerToUpdate->GetPlayerNameCustom(), PlayerToUpdate->GetScore(), PlayerToUpdate->GetDefeats());
	}
	else if (Type == EScoreboardUpdate::ESU_MAX) // player joins or leaves the game
	{
		if (AreWeInATeamsMatch()) SetHUDScoreboardTeamScores();
		else SetHUDScoreboardScores(0, 9); // hardcoded for 10 players
	}

	StereoLayer->MarkTextureForUpdate();
}

void AGunfightPlayerController::SetScoreboardVisibility(bool bVisible)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr || CharacterOverlay->Scoreboard == nullptr) return;
	
	UWidget* ScoreboardWidget = Cast<UWidget>(CharacterOverlay->Scoreboard);
	if (ScoreboardWidget == nullptr) return;

	if (bVisible) ScoreboardWidget->SetRenderOpacity(1.f);
	else ScoreboardWidget->SetRenderOpacity(0.f);
}

void AGunfightPlayerController::SetGunfightMatchState(EGunfightMatchState NewState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		GunfightMatchState = NewState;

		// Call the onrep to make sure the callbacks happen
		OnRep_GunfightMatchState();
	}
}

void AGunfightPlayerController::SetGunfightRoundMatchState(EGunfightRoundMatchState NewRoundState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		GunfightRoundMatchState = NewRoundState;

		// Call OnRep to make sure the callbacks happen
		OnRep_GunfightRoundMatchState();
	}
}

void AGunfightPlayerController::ClientPostLoginSetMatchState_Implementation(EGunfightMatchState NewState)
{
	GunfightMatchState = NewState;

	OnRep_GunfightMatchState();

	// need to update the HUD correctly once the HUD widget is created.
}

void AGunfightPlayerController::SetHUDScoreboardScores(int32 StartIndex, int32 EndIndex)
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightGameState == nullptr || GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr || StereoLayer == nullptr) return;

	int n = GunfightGameState->SortedPlayers.Num();

	for (int i = StartIndex; i <= EndIndex; ++i)
	{
		if (i >= n)
		{
			CharacterOverlay->ClearScore(i);
			continue;
		}
		const AGunfightPlayerState* CurrentPlayerState = GunfightGameState->SortedPlayers[i];
		if (CurrentPlayerState != nullptr)
		{
			CharacterOverlay->ScoreUpdate(i, CurrentPlayerState->GetPlayerNameCustom(), CurrentPlayerState->GetScore(), CurrentPlayerState->GetDefeats());
			CharacterOverlay->bIsConstructed = true;
		}
	}

	StereoLayer->MarkTextureForUpdate();

	if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString("SetHUDScoreboardScores FUCK!"));
}

void AGunfightPlayerController::SetHUDScoreboardTeamScores()
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightGameState == nullptr || GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr || StereoLayer == nullptr) return;

	if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString("Set HUD Team Scores"));

	// loop through sorted players and store all players in their respective team queues
	TQueue<AGunfightPlayerState*> BluePlayersQueue;
	TQueue<AGunfightPlayerState*> RedPlayersQueue;

	for (auto p : GunfightGameState->SortedPlayers)
	{
		if (p == nullptr) continue;
		if (p->GetTeam() == ETeam::ET_BlueTeam) BluePlayersQueue.Enqueue(p);
		else if (p->GetTeam() == ETeam::ET_RedTeam) RedPlayersQueue.Enqueue(p);
	}

	// update blue team scores
	for (int i = 0; i < 4; ++i)
	{
		AGunfightPlayerState* p = nullptr;
		BluePlayersQueue.Dequeue(p);

		if (p != nullptr)
		{
			CharacterOverlay->ScoreUpdate(i, p->GetPlayerNameCustom(), p->GetScore(), p->GetDefeats(), FColor::Blue);
		}
		else
		{
			CharacterOverlay->ClearScore(i);
		}
	}

	// update red team scores
	for (int i = 6; i < 10; ++i)
	{
		AGunfightPlayerState* p = nullptr;
		RedPlayersQueue.Dequeue(p);

		if (p != nullptr)
		{
			CharacterOverlay->ScoreUpdate(i, p->GetPlayerNameCustom(), p->GetScore(), p->GetDefeats(), FColor::Red);
		}
		else
		{
			CharacterOverlay->ClearScore(i);
		}
	}

	// clear middle scores
	CharacterOverlay->ClearScore(4);
	CharacterOverlay->ClearScore(5);
}

void AGunfightPlayerController::DrawSortedPlayers()
{
	if (GunfightGameState && GEngine && HasAuthority() && GetPawn() && GetPawn()->IsLocallyControlled())
	{
		for (auto i : GunfightGameState->SortedPlayers)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("%s: %d, %d"), *i->GetPlayerNameCustom(), FMath::FloorToInt(i->GetScore()), i->GetDefeats()));
		}
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("-----------------------------")));
	}
}

void AGunfightPlayerController::CountdownTick(float TimeLeft)
{
	if (!IsLocalController()) return;
	if (TimeLeft >= TickStartTime || TimeLeft <= 0.f) return;
	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown) return;

	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
			GunfightHUD->CharacterOverlay &&
			GunfightHUD->CharacterOverlay->MatchCountdownText;

	if (!bHUDValid) return;

	ChangeTextBlockColor(GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::Red));

	UGameplayStatics::PlaySound2D(this, TickSound);
}

void AGunfightPlayerController::PlayRoundStartVoiceline()
{
	if (!IsLocalController()) return;

	if (!bPlayingVoiceline && RoundStartVoiceline)
	{
		USceneComponent* CameraComp = GetRoundStartVoicelineCamera();

		if (CameraComp)
		{
			UGameplayStatics::SpawnSoundAttached(RoundStartVoiceline, CameraComp);
		}
		else
		{
			UGameplayStatics::PlaySound2D(this, RoundStartVoiceline);
		}
		GetWorldTimerManager().SetTimer(VoicelineTimer, this, &AGunfightPlayerController::VoicelineTimerEnd, 4.f, false, 4.f);
	}
}

void AGunfightPlayerController::VoicelineTimerEnd()
{
	bPlayingVoiceline = false;
}

USceneComponent* AGunfightPlayerController::GetRoundStartVoicelineCamera()
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightPlayerState* PState = GetPlayerState<AGunfightPlayerState>();
	if (PState == nullptr || GunfightGameState == nullptr) return nullptr;

	ETeam PlayerTeam = PState->GetTeam();

	if (PlayerTeam != ETeam::ET_RedTeam && PlayerTeam != ETeam::ET_BlueTeam)
	{
		AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
		if (GunfightCharacter == nullptr) return nullptr;

		return GunfightCharacter->VRReplicatedCamera;
	}

	TArray<AGunfightPlayerState*> TeamArray;

	// Fill TeamArray with our teammates.
	for (auto p : GunfightGameState->PlayerArray)
	{
		if (p)
		{
			AGunfightPlayerState* gp = Cast<AGunfightPlayerState>(p);
			if (gp && gp->GetTeam() == PlayerTeam)
			{
				TeamArray.Add(gp);
			}
		}
	}
	
	if (TeamArray.IsEmpty()) return nullptr;
	
	int32 RandomIndex = FMath::RandRange(0, TeamArray.Num() - 1);

	AGunfightPlayerState* TeammatePlayerState = TeamArray[RandomIndex];
	if (TeammatePlayerState == nullptr) return nullptr;

	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(TeammatePlayerState->GetPawn());
	if (GChar == nullptr) return nullptr;

	return GChar->VRReplicatedCamera;
}

void AGunfightPlayerController::ChangeTextBlockColor(UTextBlock* TextBlock, FSlateColor NewColor)
{
	if (TextBlock == nullptr) return;
	TextBlock->SetColorAndOpacity(NewColor);
}

AGunfightHUD* AGunfightPlayerController::GetGunfightHUD()
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	return GunfightHUD;
}

void AGunfightPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;

	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Uninitialized)
	{
		if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
		else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress) TimeLeft = MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
		else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown) TimeLeft = CooldownTime + MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else
	{
		if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup)
		{
			TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
		}
		else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundStart)
		{
			TimeLeft = RoundStartTime - GetServerTime() + LevelStartingTime;
		}
		else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress)
		{
			TimeLeft = RoundTime + RoundStartTime - GetServerTime() + LevelStartingTime;
		}
		else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown)
		{
			TimeLeft = RoundEndTime - GetServerTime() + LevelStartingTime;
		}
		else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
		{
			TimeLeft = CooldownTime + TotalRoundTime + RoundStartTime - GetServerTime() + LevelStartingTime;
		}
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		GunfightGameMode = GunfightGameMode == nullptr ? Cast<AGunfightGameMode>(UGameplayStatics::GetGameMode(this)) : GunfightGameMode;
		if (GunfightGameMode)
		{
			SecondsLeft = FMath::CeilToInt(GunfightGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup				|| 
			GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown		|| 
			GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup	||
			GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (GunfightMatchState	== EGunfightMatchState::EGMS_MatchInProgress		|| 
			GunfightRoundMatchState	== EGunfightRoundMatchState::EGRMS_RoundStart		|| 
			GunfightRoundMatchState	== EGunfightRoundMatchState::EGRMS_RoundInProgress	|| 
			GunfightRoundMatchState	== EGunfightRoundMatchState::EGRMS_RoundCooldown)
		{
			SetHUDMatchCountdown(TimeLeft);
			CountdownTick(TimeLeft);
		}
	}

	/*if (GEngine && HasAuthority())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, FString::Printf(TEXT("%d"), CountdownInt));
	}*/

	CountdownInt = SecondsLeft;
}

void AGunfightPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? TObjectPtr<APlayerState>(GetPlayerState<APlayerState>()) : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->HighPingAnimation &&
		GunfightHUD->CharacterOverlay->IsAnimationPlaying(GunfightHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void AGunfightPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;

		// update ping for UI
		SetHUDPing();
	}
}

void AGunfightPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AGunfightPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AGunfightPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AGunfightPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AGunfightPlayerController::PawnLeavingGame()
{
	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GetPawn());
	if (GChar)
	{
		GChar->DestroyDefaultWeapon();
	}

	Super::PawnLeavingGame();
}

void AGunfightPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		//HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		//HandleCooldown();
		//HandleCooldown2();
	}
}

void AGunfightPlayerController::OnRep_LevelStartingTime()
{
	/*if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown && GetWorld())
	{
		LevelStartingTime = GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}*/
}

void AGunfightPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		//HandleCooldown();
		//HandleCooldown2();
	}
}

void AGunfightPlayerController::HandleMatchHasStarted()
{
	bWon = false;

	if (UpdateHUD(true, false, false, false, false))
	{
		ChangeTextBlockColor(GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::White));
		//GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::White);
	}

	/*AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	if (GunfightCharacter->CharacterOverlayWidget == nullptr || GunfightCharacter->VRStereoLayer == nullptr) return;

	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay && CharacterOverlay->AnnouncementText && CharacterOverlay->WarmupTime && CharacterOverlay->InfoText && CharacterOverlay->MatchCountdownText
		&& CharacterOverlay->AnnouncementBackground && CharacterOverlay->InfoBackground && CharacterOverlay->CountdownBackground)
	{
		GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
		GunfightCharacter->VRStereoLayer->SetVisibility(true);
		CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(0.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(0.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(1.0f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(0.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(1.0f);
	}*/
}

void AGunfightPlayerController::HandleCooldown()
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD)
	{
		GunfightHUD->CharacterOverlay->RemoveFromParent();

		bool bHUDValid = GunfightHUD->Announcement &&
			GunfightHUD->Announcement->AnnouncementText &&
			GunfightHUD->Announcement->InfoText;

		if (bHUDValid)
		{
			GunfightHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			GunfightHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
			AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
			if (GunfightGameState && GunfightPlayerState)
			{
				TArray<AGunfightPlayerState*> TopPlayers = GunfightGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("Match Draw.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == GunfightPlayerState)
				{
					InfoTextString = FString("You Are The Winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerNameCustom());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players Tied For The Win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerNameCustom()));
					}
				}

				GunfightHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter && GunfightCharacter->GetCombat())
	{
		GunfightCharacter->bDisableGameplay = true;
		GunfightCharacter->GetCombat()->FireButtonPressed(false, false);
	}
}

void AGunfightPlayerController::HandleCooldown2()
{
	
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr || GunfightCharacter->CharacterOverlayWidget == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;

	// TODO Hide all components like healthbar, ammo, health, timer

	bool bHUDValid = CharacterOverlay->AnnouncementText &&
		CharacterOverlay->InfoText; /*&&
		CharacterOverlay->MatchCountdownText &&
		CharacterOverlay->InfoBackground &&
		CharacterOverlay->CountdownBackground &&
		CharacterOverlay->AnnouncementBackground;*/

	if (UpdateHUD(false, true, true, true, false))
	{
		/*CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
		CharacterOverlay->InfoText->SetRenderOpacity(1.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);

		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(1.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(1.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(0.f);*/

		FString AnnouncementText("Restarting match in:");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));

		GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
		AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
		if (GunfightGameState && GunfightPlayerState)
		{
			bWon = false;

			TArray<AGunfightPlayerState*> TopPlayers = GunfightGameState->TopScoringPlayers;
			FString InfoTextString;
			if (TopPlayers.Num() == 0)
			{
				InfoTextString = FString("There is no winner.");
			}
			else if (TopPlayers.Num() == 1 && TopPlayers[0] == GunfightPlayerState)
			{
				InfoTextString = FString("You are the winner!");

				if (IsLocalController())
				{
					bWon = true;
				}
			}
			else if (TopPlayers.Num() == 1)
			{
				InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerNameCustom());
			}
			else if (TopPlayers.Num() > 1)
			{
				InfoTextString = FString("Players tied for the win:\n");
				for (auto TiedPlayer : TopPlayers)
				{
					InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerNameCustom()));
				}
			}

			CharacterOverlay->InfoText->SetText(FText::FromString(InfoTextString));

			UpdateSaveGameData(bWon);
		}
	}
}

void AGunfightPlayerController::OnRep_GunfightMatchState()
{
	if (!HasAuthority() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, FString::Printf(TEXT("OnRep GunfightMatchState: %s"), *UEnum::GetValueAsString(GunfightMatchState)));
	}

	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GetPawn());
	if (GChar)
	{
		GChar->DebugLogMessage(FString::Printf(TEXT("OnRep GunfightMatchState: %s"), *UEnum::GetValueAsString(GunfightMatchState)));
	}

	// get TimeLeft to set the soundtrack start time
	float TimeLeft = 0.f;

	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup)
	{
		HandleGunfightWarmupStarted();
		TimeLeft = WarmupTime - (WarmupTime - GetServerTime() + LevelStartingTime);
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress)
	{
		HandleGunfightMatchStarted();
		TimeLeft = MatchTime - (MatchTime + WarmupTime - GetServerTime() + LevelStartingTime);
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown)
	{
		HandleGunfightCooldownStarted();
		TimeLeft = CooldownTime - (CooldownTime + MatchTime + WarmupTime - GetServerTime() + LevelStartingTime);
	}

	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;

	if (GunfightHUD)
	{
		GunfightHUD->UpdateSoundtrack(GunfightMatchState, TimeLeft);
	}

	//UpdateMatchSoundtrack(GunfightMatchState, TimeLeft);
	
	/*
	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress) TimeLeft = MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown) TimeLeft = CooldownTime + MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
	*/
}

void AGunfightPlayerController::OnRep_GunfightRoundMatchState()
{
	if (!HasAuthority() && GEngine && IsLocalController())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, FString::Printf(TEXT("OnRep GunfightROUNDMatchState: %s"), *UEnum::GetValueAsString(GunfightRoundMatchState)));
	}

	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup)
	{
		HandleGunfightRoundMatchStarted();
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundStart)
	{
		HandleGunfightRoundRestarted();
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress)
	{
		HandleGunfightRoundStarted();
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown)
	{
		HandleGunfightRoundCooldownStarted();
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
	{
		HandleGunfightRoundMatchEnded();
	}

	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;

	if (GunfightHUD) // turn off soundtrack for testing
	{
		GunfightHUD->UpdateSoundtrackRound(GunfightRoundMatchState);
	}
	else
	{
		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GetPawn());
		if (GChar) GunfightHUD = GChar->GunfightHUD;
		if (GEngine && IsLocalController()) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString("OnRep Round State HUD IS FALSE!!!!"));
	}
}

void AGunfightPlayerController::HandleGunfightWarmupStarted()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr || GunfightCharacter->CharacterOverlayWidget == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;

	if (HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World == nullptr) return;
		LevelStartingTime = World->TimeSeconds;
	}

	/*
	UWorld* World = GetWorld();
	if (World == nullptr) return;
if (!HasAuthority() && World->TimeSeconds > 30) 
	{
		ServerCheckMatchState();
	}
	else if (HasAuthority() && World->TimeSeconds > 30)
	{
		LevelStartingTime = World->TimeSeconds;
	}*/

	if (UpdateHUD(false, true, true, false, false))
	{
		FString AnnouncementText("Warmup");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));
	}

	// TODO Hide all components like healthbar, ammo, health, timer
	/*
	bool bHUDValid = CharacterOverlay->AnnouncementText; &&
		CharacterOverlay->WarmupTime &&
		CharacterOverlay->InfoText &&
		CharacterOverlay->MatchCountdownText &&
		CharacterOverlay->AnnouncementBackground &&
		CharacterOverlay->InfoBackground && 
		CharacterOverlay->CountdownBackground;

	if (bHUDValid)
	{*/
		/*CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);

		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(1.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(0.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(0.f);

		FString AnnouncementText("Warmup");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));
	}*/
}

void AGunfightPlayerController::HandleGunfightMatchStarted()
{
	HandleMatchHasStarted();
	PlayRoundStartVoiceline();
}

void AGunfightPlayerController::HandleGunfightCooldownStarted()
{
	HandleCooldown2();
}

void AGunfightPlayerController::HandleGunfightRoundMatchStarted()
{
	bWon = false;

	if (HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World == nullptr) return;
		LevelStartingTime = World->TimeSeconds;

		AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
		if (GunfightCharacter == nullptr) return;
		GunfightCharacter->bDisableShooting = false;
	}

	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	if (GunfightCharacter->CharacterOverlayWidget == nullptr || GunfightCharacter->VRStereoLayer == nullptr) return;

	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	CharacterOverlay = CharacterOverlay == nullptr ? GunfightHUD->CharacterOverlay : CharacterOverlay;

	if (UpdateHUD(false, true, true, false, true) && CharacterOverlay	&& CharacterOverlay->SwapText)
	{
		/*CharacterOverlay->WarmupTime			&& 
		CharacterOverlay->InfoText				&& 
		CharacterOverlay->MatchCountdownText	&&
		CharacterOverlay->RedScoreText			&&
		CharacterOverlay->BlueScoreText			&&*/
		/*&&
		CharacterOverlay->AnnouncementBackground &&
		CharacterOverlay->InfoBackground &&
		CharacterOverlay->CountdownBackground &&
		CharacterOverlay->BlueBackground &&
		CharacterOverlay->RedBackground)
	{
		GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
		GunfightCharacter->VRStereoLayer->SetVisibility(true);
		CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);
		CharacterOverlay->RedScoreText->SetRenderOpacity(1.0f);
		CharacterOverlay->BlueScoreText->SetRenderOpacity(1.0f);

		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(1.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(0.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(0.f);
		CharacterOverlay->BlueBackground->SetRenderOpacity(1.f);
		CharacterOverlay->RedBackground->SetRenderOpacity(1.f);*/

		CharacterOverlay->SwapText->SetRenderOpacity(1.0f);
		CharacterOverlay->SwapText->SetText(FText::FromString(FString("Press and hold right stick to request a team swap.")));

		FString AnnouncementText("Warmup");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		CharacterOverlay->AnnouncementText->SetColorAndOpacity(DefaultAnnouncementColor);
	}
}

void AGunfightPlayerController::HandleGunfightRoundRestarted()
{
	if (HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World == nullptr) return;
		LevelStartingTime = World->TimeSeconds;

		AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
		if (GunfightCharacter == nullptr) return;
		GunfightCharacter->bDisableShooting = true;
	}

	if (UpdateHUD(true, false, false, false, true))
	{
		ChangeTextBlockColor(GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::White));
		UpdateAnnouncement(false, DefaultAnnouncementColor, FString(""));
	}

	/*
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	if (GunfightCharacter->CharacterOverlayWidget == nullptr || GunfightCharacter->VRStereoLayer == nullptr) return;

	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay &&
		CharacterOverlay->AnnouncementText &&
		CharacterOverlay->WarmupTime &&
		CharacterOverlay->InfoText &&
		CharacterOverlay->MatchCountdownText &&
		CharacterOverlay->RedScoreText &&
		CharacterOverlay->BlueScoreText && 
		CharacterOverlay->SwapText &&
		CharacterOverlay->AnnouncementBackground &&
		CharacterOverlay->InfoBackground &&
		CharacterOverlay->CountdownBackground && 
		CharacterOverlay->BlueBackground &&
		CharacterOverlay->RedBackground)
	{
		GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
		GunfightCharacter->VRStereoLayer->SetVisibility(true);
		CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(0.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(1.f);
		CharacterOverlay->RedScoreText->SetRenderOpacity(1.0f);
		CharacterOverlay->BlueScoreText->SetRenderOpacity(1.0f);
		CharacterOverlay->SwapText->SetRenderOpacity(0.f);

		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(0.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(0.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(1.f);
		CharacterOverlay->BlueBackground->SetRenderOpacity(1.f);
		CharacterOverlay->RedBackground->SetRenderOpacity(1.f);
	}*/
}

void AGunfightPlayerController::HandleGunfightRoundStarted()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	GunfightCharacter->SetDisableMovement(false);

	if (UpdateHUD(true, false, false, false, true))
	{
		ChangeTextBlockColor(GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::White));
	}

	if (HasAuthority())
	{
		GunfightCharacter->bDisableShooting = false;
	}

	PlayRoundStartVoiceline();

	if (IsLocalController() && IsScoreNil())
	{
		ShowAnnouncementForDuration(FSlateColor(FColor::White), FString("MATCH START"), 3.5f);
		//UpdateAnnouncement(true, FSlateColor(FColor::White), FString("MATCH START"));
	}

	/*
	if (GunfightCharacter->CharacterOverlayWidget == nullptr || GunfightCharacter->VRStereoLayer == nullptr) return;

	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay &&
		CharacterOverlay->AnnouncementText &&
		CharacterOverlay->WarmupTime &&
		CharacterOverlay->InfoText &&
		CharacterOverlay->MatchCountdownText &&
		CharacterOverlay->RedScoreText &&
		CharacterOverlay->BlueScoreText && 
		CharacterOverlay->AnnouncementBackground &&
		CharacterOverlay->InfoBackground &&
		CharacterOverlay->CountdownBackground &&
		CharacterOverlay->BlueBackground &&
		CharacterOverlay->RedBackground)
	{
		GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
		GunfightCharacter->VRStereoLayer->SetVisibility(true);
		CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(0.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(1.f);
		CharacterOverlay->RedScoreText->SetRenderOpacity(1.0f);
		CharacterOverlay->BlueScoreText->SetRenderOpacity(1.0f);
		CharacterOverlay->BlueBackground->SetRenderOpacity(1.f);
		CharacterOverlay->RedBackground->SetRenderOpacity(1.f);

		CharacterOverlay->AnnouncementBackground->SetRenderOpacity(0.f);
		CharacterOverlay->InfoBackground->SetRenderOpacity(0.f);
		CharacterOverlay->CountdownBackground->SetRenderOpacity(1.f);
	}*/
}

void AGunfightPlayerController::HandleGunfightRoundEnded()
{
	
}

void AGunfightPlayerController::HandleGunfightRoundCooldownStarted()
{
	if (HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World == nullptr) return;
		LevelStartingTime = World->TimeSeconds;
	}

	if (UpdateHUD(true, false, true, false, true))
	{
		ChangeTextBlockColor(GunfightHUD->CharacterOverlay->MatchCountdownText, FSlateColor(FColor::White));
	}
}

void AGunfightPlayerController::HandleGunfightRoundMatchEnded()
{
	AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightPlayerState == nullptr || GunfightGameState == nullptr || GunfightHUD == nullptr) return;

	TotalRoundTime = RoundTime - (RoundTime + RoundStartTime - GetServerTime() + LevelStartingTime);
	
	/*UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;*/

	bool bHUDValid = GunfightHUD->CharacterOverlay			&&
		GunfightHUD->StereoLayer							&&
		GunfightHUD->CharacterOverlay->InfoText				&&
		GunfightHUD->CharacterOverlay->AnnouncementText		&&
		GunfightHUD->CharacterOverlay->WarmupTime			&&
		GunfightHUD->CharacterOverlay->MatchCountdownText &&
		CharacterOverlay->AnnouncementBackground &&
		CharacterOverlay->InfoBackground &&
		CharacterOverlay->CountdownBackground;

	if (bHUDValid == false) return;

	CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
	CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
	CharacterOverlay->InfoText->SetRenderOpacity(1.f);
	CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);

	CharacterOverlay->AnnouncementBackground->SetRenderOpacity(1.f);
	CharacterOverlay->InfoBackground->SetRenderOpacity(1.f);
	CharacterOverlay->CountdownBackground->SetRenderOpacity(0.f);

	FString AnnouncementText = FString("Restarting in:");
	CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));

	bWon = false;
	FString InfoTextString;
	//InfoTextString = GunfightPlayerState->GetTeam() == GunfightGameState->GetWinningTeam() ? FString("VICTORY") : FString("DEFEAT");

	FSlateColor InfoTextColor;
	FLinearColor InfoBackgroundColor;

	if (GunfightPlayerState->GetTeam() == GunfightGameState->GetWinningTeam())
	{
		bWon = true;
		InfoTextString = FString("VICTORY");
		InfoTextColor = FSlateColor(FColor::Green);
		InfoBackgroundColor = FLinearColor(0.f, 1.f, 0.f, 0.5f);
		UGameplayStatics::PlaySound2D(this, MatchWinVoiceline);
	}
	else
	{
		InfoTextString = FString("DEFEAT");
		InfoTextColor = FSlateColor(FColor::Orange);
		InfoBackgroundColor = FLinearColor(1.f, 0.f, 0.f, 0.5f);
		UGameplayStatics::PlaySound2D(this, MatchLossVoiceline);
	}

	CharacterOverlay->InfoText->SetText(FText::FromString(InfoTextString));
	CharacterOverlay->InfoText->SetColorAndOpacity(InfoTextColor);
	CharacterOverlay->InfoBackground->SetBrushTintColor(FSlateColor(InfoBackgroundColor));
	UpdateSaveGameData(bWon);
}

void AGunfightPlayerController::UpdateSaveGameData(bool bWonTheGame)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance == nullptr) return;
	UGunfightGameInstanceSubsystem* GunfightSubsystem = GameInstance->GetSubsystem<UGunfightGameInstanceSubsystem>();
	if (GunfightSubsystem == nullptr) return;

	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	
	if (bWonTheGame)
	{
		GunfightSubsystem->AddToWins();
	}

	if (UGameplayStatics::SaveGameToSlot(GunfightSubsystem->GunfightSaveGame, GunfightSubsystem->GunfightSaveGame->SaveSlotName, 0)) 
	{
		// Save succeeded
		if (GunfightCharacter == nullptr) return;
		GunfightCharacter->DebugLogMessage(FString("UpdateSaveGameData Gunfight save succeeded"));
	}
	else
	{
		// Save failed
		if (GunfightCharacter == nullptr) return;
		GunfightCharacter->DebugLogMessage(FString("UpdateSaveGameData Gunfight save failed"));
	}
}

bool AGunfightPlayerController::UpdateHUD(bool bMatchCountdown, bool bWarmupTime, bool bAnnouncement, bool bInfo, bool bTeamScores)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr || GunfightHUD->CharacterOverlay == nullptr) return false;
	UCharacterOverlay* CharOverlay = GunfightHUD->CharacterOverlay;

	bool bHUDValid = CharOverlay->MatchCountdownText	&&
		CharOverlay->CountdownBackground				&&
		CharOverlay->WarmupTime							&&
		CharOverlay->AnnouncementText					&&
		CharOverlay->AnnouncementBackground				&&
		CharOverlay->InfoText							&&
		CharOverlay->InfoBackground						&&
		CharOverlay->BlueScoreText						&&
		CharOverlay->BlueBackground						&&
		CharOverlay->RedScoreText						&&
		CharOverlay->RedBackground;

	if (!bHUDValid) return false;

	float Opacity = bMatchCountdown;
	CharOverlay->MatchCountdownText->SetRenderOpacity(Opacity);
	CharOverlay->CountdownBackground->SetRenderOpacity(Opacity);

	Opacity = bWarmupTime;
	CharOverlay->WarmupTime->SetRenderOpacity(Opacity);

	Opacity = bAnnouncement;
	CharOverlay->AnnouncementText->SetRenderOpacity(Opacity);
	CharOverlay->AnnouncementBackground->SetRenderOpacity(Opacity);

	Opacity = bInfo;
	CharOverlay->InfoText->SetRenderOpacity(Opacity);
	CharOverlay->InfoBackground->SetRenderOpacity(Opacity);

	Opacity = bTeamScores;
	CharOverlay->BlueScoreText->SetRenderOpacity(Opacity);
	CharOverlay->BlueBackground->SetRenderOpacity(Opacity);
	CharOverlay->RedScoreText->SetRenderOpacity(Opacity);
	CharOverlay->RedBackground->SetRenderOpacity(Opacity);

	return true;
}

void AGunfightPlayerController::ShowAnnouncementForDuration(const FSlateColor& Color, const FString& AnnouncementString, float Duration)
{
	if (IsLocalController() == false) return;

	UpdateAnnouncement(true, Color, AnnouncementString);
	GetWorldTimerManager().SetTimer(AnnouncementTimer, this, &AGunfightPlayerController::AnnouncementTimerFinished, Duration, false);
}

void AGunfightPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;

	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->HealthBar &&
		GunfightHUD->CharacterOverlay->HealthText &&
		StereoLayer;

	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		GunfightHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		//FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		GunfightHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
		const float HealthBlueColor = FMath::GetMappedRangeValueClamped(FVector2d(50, 100.f), FVector2d(0, 255.f), Health);
		const float HealthGreenColor = Health >= 50.f ? 255.f : FMath::GetMappedRangeValueClamped(FVector2d(25.f, 50.f), FVector2d(0.f, 255.f), Health);
		GunfightHUD->CharacterOverlay->HealthText->SetColorAndOpacity(FSlateColor(FLinearColor(255.f, HealthGreenColor, HealthBlueColor)));
		StereoLayer->MarkTextureForUpdate();
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AGunfightPlayerController::SetHUDScore(float Score)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->ScoreAmount &&
		StereoLayer;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		GunfightHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
		StereoLayer->MarkTextureForUpdate();
		//UpdateScoreboard();
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AGunfightPlayerController::SetHUDTeamScore(float Score, ETeam TeamToUpdate)
{
	AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = TeamToUpdate != ETeam::ET_NoTeam &&
		GunfightHUD &&
		GunfightPlayerState &&
		GunfightGameState &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->RedScoreText &&
		GunfightHUD->CharacterOverlay->BlueScoreText &&
		GunfightHUD->CharacterOverlay->AnnouncementText &&
		StereoLayer;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		UTextBlock* TextToUpdate = TeamToUpdate == ETeam::ET_RedTeam ? GunfightHUD->CharacterOverlay->RedScoreText : GunfightHUD->CharacterOverlay->BlueScoreText;
		TextToUpdate->SetText(FText::FromString(ScoreText));
		StereoLayer->MarkTextureForUpdate();
		//UpdateScoreboard();

		float TeamScore = TeamToUpdate == ETeam::ET_RedTeam ? GunfightGameState->RedTeamScore : GunfightGameState->BlueTeamScore;

		if (!GunfightGameState->IsMatchEnding() && TeamScore >= 1)
		{
			//FString AnnouncementString = TeamToUpdate == GunfightPlayerState->GetTeam() ? FString("ROUND WON") : FString("ROUND LOST");
			FString AnnouncementString;
			FColor AnnouncementColor;
			bool bWonRound = TeamToUpdate == GunfightPlayerState->GetTeam();
			if (bWonRound)
			{
				AnnouncementString = FString("ROUND WON");
				AnnouncementColor = FColor::Green;
				UGameplayStatics::PlaySound2D(this, RoundWinVoiceline);
			}
			else
			{
				AnnouncementString = FString("ROUND LOST");
				AnnouncementColor = FColor::Orange;
				UGameplayStatics::PlaySound2D(this, RoundLossVoiceline);
			}

			UpdateAnnouncement(true, FSlateColor(AnnouncementColor), AnnouncementString);
		}
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AGunfightPlayerController::SetHUDDefeats(int32 Defeats)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->DefeatsAmount &&
		StereoLayer;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		GunfightHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
		StereoLayer->MarkTextureForUpdate();
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AGunfightPlayerController::SetHUDWeaponAmmo(int32 Ammo, bool bLeft)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("SetHUDAmmo. Ammo: %d"), Ammo));
	
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (!GunfightHUD)
	{
		AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GetPawn());
		if (GChar)
		{
			GunfightHUD = GChar->GunfightHUD;
		}
	}

	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->WeaponAmmoAmount &&
		GunfightHUD->CharacterOverlay->WeaponAmmoAmountLeft;
		// && GunfightHUD->StereoLayer;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);

		UTextBlock* CurrentAmmoText = bLeft ? GunfightHUD->CharacterOverlay->WeaponAmmoAmountLeft : GunfightHUD->CharacterOverlay->WeaponAmmoAmount;
		if (CurrentAmmoText == nullptr) return;
		CurrentAmmoText->SetText(FText::FromString(AmmoText));
		//GunfightHUD->StereoLayer->MarkTextureForUpdate();
	}
	else
	{
		bInitializeWeaponAmmo = true;

		bLeft ? HUDWeaponAmmoLeft = Ammo : HUDWeaponAmmoRight = Ammo;
		/*
		float* CurrentHUDWeaponAmmo = bLeft ? &HUDWeaponAmmoLeft : &HUDWeaponAmmoRight;
		if (CurrentHUDWeaponAmmo == nullptr) return;
		*CurrentHUDWeaponAmmo = Ammo;*/
	}

}

void AGunfightPlayerController::SetHUDCarriedAmmo(int32 Ammo, bool bLeft)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->CarriedAmmoAmount &&
		GunfightHUD->CharacterOverlay->CarriedAmmoAmountLeft &&
		StereoLayer;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);

		UTextBlock* CurrentCarriedAmmoText = bLeft ? GunfightHUD->CharacterOverlay->CarriedAmmoAmountLeft : GunfightHUD->CharacterOverlay->CarriedAmmoAmount;

		if (CurrentCarriedAmmoText == nullptr) return;
		CurrentCarriedAmmoText->SetText(FText::FromString(AmmoText));
		StereoLayer->MarkTextureForUpdate();
	}
	else
	{
		bInitializeCarriedAmmo = true;
		bLeft ? HUDCarriedAmmoLeft = Ammo : HUDCarriedAmmoRight = Ammo;
		/*
		float* CurrentHUDCarriedAmmo = bLeft ? &HUDCarriedAmmoLeft : &HUDCarriedAmmoRight;
		*CurrentHUDCarriedAmmo = Ammo;*/
	}
}

void AGunfightPlayerController::SetHUDWeaponAmmoVisible(bool bLeft, bool bNewVisible)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->WeaponAmmoAmount &&
		GunfightHUD->CharacterOverlay->WeaponAmmoAmountLeft &&
		GunfightHUD->CharacterOverlay->CarriedAmmoAmount &&
		GunfightHUD->CharacterOverlay->CarriedAmmoAmountLeft &&
		GunfightHUD->CharacterOverlay->AmmoSlashR &&
		GunfightHUD->CharacterOverlay->AmmoSlashL &&
		StereoLayer;

	if(bHUDValid)
	{
		UTextBlock* CurrentAmmoText = nullptr;
		UTextBlock* CurrentCarriedText = nullptr;
		UTextBlock* CurrentSlashText = nullptr;

		if (bLeft)
		{
			CurrentAmmoText = GunfightHUD->CharacterOverlay->WeaponAmmoAmountLeft;
			CurrentCarriedText = GunfightHUD->CharacterOverlay->CarriedAmmoAmountLeft;
			CurrentSlashText = GunfightHUD->CharacterOverlay->AmmoSlashL;
		}
		else
		{
			CurrentAmmoText = GunfightHUD->CharacterOverlay->WeaponAmmoAmount;
			CurrentCarriedText = GunfightHUD->CharacterOverlay->CarriedAmmoAmount;
			CurrentSlashText = GunfightHUD->CharacterOverlay->AmmoSlashR;
		}
		if (CurrentAmmoText == nullptr || CurrentCarriedText == nullptr || CurrentSlashText == nullptr) return;

		CurrentAmmoText->SetRenderOpacity(bNewVisible);
		CurrentCarriedText->SetRenderOpacity(bNewVisible);
		CurrentSlashText->SetRenderOpacity(bNewVisible);

		StereoLayer->MarkTextureForUpdate();
	}
}

void AGunfightPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->MatchCountdownText &&
		StereoLayer;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			GunfightHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		GunfightHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
		StereoLayer->MarkTextureForUpdate();
	}
}

void AGunfightPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	/*
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->Announcement &&
		GunfightHUD->Announcement->WarmupTime;
	*/
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	
	bool bHUDValid = CharacterOverlay &&
		CharacterOverlay->WarmupTime &&
		StereoLayer;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			/*if (!HasAuthority() && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, FString::Printf(TEXT("TimeLeft CLIENT: %f"), CountdownTime));
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Emerald, FString::Printf(TEXT("ClientServerDelta: %f"), ClientServerDelta));
			}
			else if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, FString::Printf(TEXT("TimeLeft Server: %f"), CountdownTime));
			}*/
			CharacterOverlay->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CharacterOverlay->WarmupTime->SetText(FText::FromString(CountdownText));
		StereoLayer->MarkTextureForUpdate();
	}
}

void AGunfightPlayerController::SetHUDPing()
{
	// could call a server RPC to update ping for all players and show it on scoreboard...

	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->PingText &&
		StereoLayer;

	AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();

	if (bHUDValid && GunfightPlayerState)
	{
		//int32 Ping = (int)SingleTripTime * 2;
		int32 Ping = GunfightPlayerState->GetCompressedPing() * 4;
		FString PingText = FString::Printf(TEXT("%dms"), Ping);
		GunfightHUD->CharacterOverlay->PingText->SetText(FText::FromString(PingText));
	}
}

void AGunfightPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(InPawn);
	if (GunfightCharacter)
	{
		GunfightHUDInitialized = GunfightCharacter->GunfightHUD;
		SetHUDHealth(GunfightCharacter->GetHealth(), GunfightCharacter->GetMaxHealth());
	}
}

void AGunfightPlayerController::ServerCheckMatchState_Implementation()
{
	AGunfightGameMode* GameMode = Cast<AGunfightGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WaitingToStartTime = GameMode->WaitingToStartTime;
		MatchTime = GameMode->GunfightMatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		WarmupTime = GameMode->GunfightWarmupTime;
		MatchEndTime = GameMode->GunfightCooldownTime;

		RoundStartTime = GameMode->GunfightRoundStartTime;
		RoundTime = GameMode->GunfightRoundTime;
		RoundEndTime = GameMode->GunfightRoundEndTime;
		
		SetGunfightMatchState(GameMode->GetGunfightMatchState());

		ClientJoinMidGame(GunfightMatchState, WarmupTime, MatchTime, MatchEndTime, LevelStartingTime, RoundStartTime, RoundTime, RoundEndTime);
	}
}

void AGunfightPlayerController::ClientJoinMidGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime, float NewRoundStartTime, float NewRoundTime, float NewRoundEndTime)
{
	WarmupTime = WaitingToStart;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;

	RoundStartTime = NewRoundStartTime;
	RoundTime = NewRoundTime;
	RoundEndTime = NewRoundEndTime;

	SetGunfightMatchState(StateOfMatch);
}

bool AGunfightPlayerController::IsScoreNil()
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	if (GunfightGameState == nullptr) return false;

	return (GunfightGameState->RedTeamScore + GunfightGameState->BlueTeamScore) == 0;
}

void AGunfightPlayerController::AnnouncementTimerFinished()
{
	if (IsLocalController() == false) return;

	UpdateAnnouncement(false);
}

bool AGunfightPlayerController::AreWeInATeamsMatch()
{
	AGunfightPlayerState* PState = GetPlayerState<AGunfightPlayerState>();
	if (PState == nullptr) return false;

	return PState->GetTeam() <= ETeam::ET_BlueTeam;
}

void AGunfightPlayerController::TryToInitScoreboard()
{

}

void AGunfightPlayerController::ClientRestartGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime)
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	World->TimeSeconds = 0.f;

	WarmupTime = WaitingToStart;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;

	GunfightMatchState = StateOfMatch;
	OnRep_GunfightMatchState();
}

void AGunfightPlayerController::ClientUpdateMatchState_Implementation(EGunfightMatchState StateOfMatch)
{
	GunfightMatchState = StateOfMatch;
	OnRep_GunfightMatchState();
}

void AGunfightPlayerController::Destroyed()
{
	ServerLeaveGame();

	Super::Destroyed();
}

void AGunfightPlayerController::ServerLeaveGame_Implementation()
{
	GunfightGameMode = GunfightGameMode == nullptr ? Cast<AGunfightGameMode>(UGameplayStatics::GetGameMode(this)) : GunfightGameMode;
	AGunfightPlayerState* PState = GetPlayerState<AGunfightPlayerState>();
	if (GunfightGameMode && PState)
	{
		GunfightGameMode->PlayerLeftGame(PState);
	}
}

void AGunfightPlayerController::HighPingWarning()
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->HighPingImage &&
		GunfightHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid)
	{
		GunfightHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		GunfightHUD->CharacterOverlay->PlayAnimation(GunfightHUD->CharacterOverlay->HighPingAnimation, 0.f, 5.f);
	}
}

void AGunfightPlayerController::StopHighPingWarning()
{
	//GunfightHUD = GunfightHUDInitialized;
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->HighPingImage &&
		GunfightHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid)
	{
		GunfightHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (GunfightHUD->CharacterOverlay->IsAnimationPlaying(GunfightHUD->CharacterOverlay->HighPingAnimation))
		{
			GunfightHUD->CharacterOverlay->StopAnimation(GunfightHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void AGunfightPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void AGunfightPlayerController::SortPlayersByScore(TArray<TObjectPtr<APlayerState>>& Players)
{
	for (auto& ThisPlayer : Players)
	{
		BinaryInsertPlayer(ThisPlayer, Players);
	}
}

void AGunfightPlayerController::BinaryInsertPlayer(TObjectPtr<APlayerState>& ThisPlayer, TArray<TObjectPtr<APlayerState>>& Players)
{
	const float PlayerScore = ThisPlayer->GetScore();
	const int SortedNum = PlayersSorted.Num();

	if (SortedNum == 0)
	{
		PlayersSorted.Add(ThisPlayer);
		return;
	}
	else if (PlayerScore > PlayersSorted[0]->GetScore())
	{
		PlayersSorted.Insert(ThisPlayer, 0);
		return;
	}
	else if (PlayerScore <= PlayersSorted[SortedNum - 1]->GetScore())
	{
		PlayersSorted.Add(ThisPlayer);
		return;
	}

	int Min = 0;
	int Max = SortedNum - 1;
	int Mid;

	while (1)
	{
		Mid = (Min + Max) / 2;
		const float CurrentScore = Players[Mid]->GetScore();

		if (PlayerScore == CurrentScore)
		{
			if (Mid == Players.Num() - 1)
			{
				PlayersSorted.Add(ThisPlayer);
			}
			else
			{
				PlayersSorted.Insert(ThisPlayer, Mid + 1);
			}
			return;
		}
		else if (Mid == Min || Mid == Max)
		{
			if (PlayerScore > CurrentScore)
			{
				PlayersSorted.Insert(ThisPlayer, Mid);
			}
			else
			{
				if (Mid == Players.Num() - 1)
				{
					PlayersSorted.Add(ThisPlayer);
				}
				else
				{
					PlayersSorted.Insert(ThisPlayer, Mid + 1);
				}
			}
			return;
		}

		if (PlayerScore < CurrentScore)
		{
			Max = Mid;
		}
		else
		{
			Min = Mid;
		}
	}
}
