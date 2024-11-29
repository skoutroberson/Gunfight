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
}

void AGunfightPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
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
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
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
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
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
		HandleCooldown();
	}
}

void AGunfightPlayerController::HandleMatchHasStarted()
{
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	if (GunfightHUD)
	{
		GunfightHUD->AddCharacterOverlay();
		if (GunfightHUD->Announcement)
		{
			GunfightHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
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

			AGunfightGameState* GunfightGameState = Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this));
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
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		GunfightHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
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
	GunfightHUD = GunfightHUD == nullptr ? Cast<AGunfightHUD>(GetHUD()) : GunfightHUD;
	bool bHUDValid = GunfightHUD &&
		GunfightHUD->Announcement &&
		GunfightHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			GunfightHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		GunfightHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
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
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void AGunfightPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (GunfightHUD && MatchState == MatchState::WaitingToStart)
	{
		GunfightHUD->AddAnnouncement();
	}
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
