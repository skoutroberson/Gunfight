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
		if (GunfightHUD && GunfightHUD->CharacterOverlay)
		{
			CharacterOverlay = GunfightHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);

				AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
				if (GunfightCharacter && GunfightCharacter->GetDefaultWeapon())
				{
					GunfightCharacter->GetDefaultWeapon()->SetHUDAmmo();
				}

				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
			}
		}
	}
	else if (!CharacterOverlay->bIsConstructed)
	{
		CharacterOverlay->FillPlayers();
	}
	else if (!bPlayerStateInitialized && GetPlayerState<AGunfightPlayerState>())
	{
		bPlayerStateInitialized = true;
		UpdateScoreboard(GetPlayerState<AGunfightPlayerState>(), EScoreboardUpdate::ESU_MAX);
	}
}

void AGunfightPlayerController::InitializeHUD()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter && GunfightHUD)
	{
		CharacterOverlay = Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject());
		if (CharacterOverlay)
		{
			GunfightHUD->CharacterOverlay = CharacterOverlay;
			//SetHUDScoreboardScores(0, 9); // hardcoded 10 players
		}
	}
}

void AGunfightPlayerController::UpdateScoreboard(AGunfightPlayerState* PlayerToUpdate, EScoreboardUpdate Type)
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());

	if (GunfightCharacter == nullptr || GunfightGameState == nullptr) return;
	if (PlayerToUpdate == nullptr && Type < EScoreboardUpdate::ESU_Join) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;
	if (CharacterOverlay->ScoreboardInfos.IsEmpty()) return;

	if (Type == EScoreboardUpdate::ESU_Score) // player gets a kill
	{
		int32 IndexToUpdate = GunfightGameState->PlayerScoreUpdate(PlayerToUpdate);
		if (GunfightGameState->ScoringPlayerIndex != INDEX_NONE)
		{
			SetHUDScoreboardScores(IndexToUpdate, GunfightGameState->ScoringPlayerIndex + 1);
		}
	}
	else if (Type == EScoreboardUpdate::ESU_Death) // player dies
	{
		int32 IndexToUpdate = GunfightGameState->SortedPlayers.Find(PlayerToUpdate);
		CharacterOverlay->ScoreUpdate(IndexToUpdate, PlayerToUpdate->GetPlayerName(), PlayerToUpdate->GetScore(), PlayerToUpdate->GetDefeats());
	}
	else if (Type == EScoreboardUpdate::ESU_MAX) // player joins or leaves the game
	{
		SetHUDScoreboardScores(0, 9); // hardcoded for 10 players
	}
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

void AGunfightPlayerController::SetHUDScoreboardScores(int32 StartIndex, int32 EndIndex)
{
	GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightGameState == nullptr || GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;

	for (int i = StartIndex; i <= EndIndex; ++i)
	{
		if (i >= GunfightGameState->SortedPlayers.Num()) return;
		const AGunfightPlayerState* CurrentPlayerState = GunfightGameState->SortedPlayers[i];
		if (CurrentPlayerState != nullptr)
		{
			CharacterOverlay->ScoreUpdate(i, CurrentPlayerState->GetPlayerName(), CurrentPlayerState->GetScore(), CurrentPlayerState->GetDefeats());
			CharacterOverlay->bIsConstructed = true;
		}
	}
}

void AGunfightPlayerController::DrawSortedPlayers()
{
	if (GunfightGameState && GEngine && HasAuthority() && GetPawn() && GetPawn()->IsLocallyControlled())
	{
		for (auto i : GunfightGameState->SortedPlayers)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("%s: %d, %d"), *i->GetPlayerName(), FMath::FloorToInt(i->GetScore()), i->GetDefeats()));
		}
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("-----------------------------")));
	}
}

AGunfightHUD* AGunfightPlayerController::GetGunfightHUD()
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD<AGunfightHUD>()) : GunfightHUD;
	return GunfightHUD;
}

void AGunfightPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;

	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress) TimeLeft = MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown) TimeLeft = CooldownTime + MatchTime + WarmupTime - GetServerTime() + LevelStartingTime;

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
		if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup || GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

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
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
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
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;

	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD == nullptr) return;

	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay && CharacterOverlay->AnnouncementText && CharacterOverlay->WarmupTime && CharacterOverlay->InfoText && CharacterOverlay->MatchCountdownText)
	{
		GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
		GunfightCharacter->VRStereoLayer->SetVisibility(true);
		CharacterOverlay->AnnouncementText->SetRenderOpacity(0.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(0.f);
		CharacterOverlay->InfoText->SetRenderOpacity(0.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(1.0f);
	}
}

void AGunfightPlayerController::HandleCooldown()
{
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
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == GunfightPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
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
	if (GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;

	// TODO Hide all components like healthbar, ammo, health, timer

	bool bHUDValid = CharacterOverlay->AnnouncementText &&
		CharacterOverlay->WarmupTime &&
		CharacterOverlay->InfoText &&
		CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
		CharacterOverlay->InfoText->SetRenderOpacity(1.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);

		FString AnnouncementText("New Match Starts In:");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));

		GunfightGameState = GunfightGameState == nullptr ? Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this)) : GunfightGameState;
		AGunfightPlayerState* GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
		if (GunfightGameState && GunfightPlayerState)
		{
			TArray<AGunfightPlayerState*> TopPlayers = GunfightGameState->TopScoringPlayers;
			FString InfoTextString;
			if (TopPlayers.Num() == 0)
			{
				InfoTextString = FString("There is no winner.");
			}
			else if (TopPlayers.Num() == 1 && TopPlayers[0] == GunfightPlayerState)
			{
				InfoTextString = FString("You are the winner!");
			}
			else if (TopPlayers.Num() == 1)
			{
				InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
			}
			else if (TopPlayers.Num() > 1)
			{
				InfoTextString = FString("Players tied for the win:\n");
				for (auto TiedPlayer : TopPlayers)
				{
					InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
				}
			}

			CharacterOverlay->InfoText->SetText(FText::FromString(InfoTextString));
		}
	}
}

void AGunfightPlayerController::OnRep_GunfightMatchState()
{
	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup)
	{
		HandleGunfightWarmupStarted();
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress)
	{
		HandleGunfightMatchStarted();
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown)
	{
		HandleGunfightCooldownStarted();
	}
}

void AGunfightPlayerController::HandleGunfightWarmupStarted()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GunfightCharacter == nullptr) return;
	CharacterOverlay = CharacterOverlay == nullptr ? Cast<UCharacterOverlay>(GunfightCharacter->CharacterOverlayWidget->GetUserWidgetObject()) : CharacterOverlay;
	if (CharacterOverlay == nullptr) return;

	// TODO Hide all components like healthbar, ammo, health, timer

	bool bHUDValid = CharacterOverlay->AnnouncementText &&
		CharacterOverlay->WarmupTime &&
		CharacterOverlay->InfoText &&
		CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		CharacterOverlay->AnnouncementText->SetRenderOpacity(1.f);
		CharacterOverlay->WarmupTime->SetRenderOpacity(1.f);
		CharacterOverlay->MatchCountdownText->SetRenderOpacity(0.f);

		FString AnnouncementText("Match starts in:");
		CharacterOverlay->AnnouncementText->SetText(FText::FromString(AnnouncementText));
	}
}

void AGunfightPlayerController::HandleGunfightMatchStarted()
{
	HandleMatchHasStarted();
}

void AGunfightPlayerController::HandleGunfightCooldownStarted()
{
	HandleCooldown2();
}

void AGunfightPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;

	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->HealthBar &&
		GunfightHUD->CharacterOverlay->HealthText;

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
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		GunfightHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
		//UpdateScoreboard();
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AGunfightPlayerController::SetHUDDefeats(int32 Defeats)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		GunfightHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void AGunfightPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		GunfightHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}

}

void AGunfightPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		GunfightHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void AGunfightPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->CharacterOverlay &&
		GunfightHUD->CharacterOverlay->MatchCountdownText;
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
		CharacterOverlay->WarmupTime;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			CharacterOverlay->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		CharacterOverlay->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void AGunfightPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(InPawn);
	if (GunfightCharacter)
	{
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
		
		SetGunfightMatchState(GameMode->GetGunfightMatchState());

		ClientJoinMidGame(GunfightMatchState, WarmupTime, MatchTime, MatchEndTime, LevelStartingTime);
	}
}

void AGunfightPlayerController::ClientJoinMidGame_Implementation(EGunfightMatchState StateOfMatch, float WaitingToStart, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = WaitingToStart;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	SetGunfightMatchState(StateOfMatch);

	/*
	if (GunfightHUD && MatchState == MatchState::WaitingToStart)
	{
		GunfightHUD->AddAnnouncement();
	}
	*/
}

void AGunfightPlayerController::HighPingWarning()
{
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
