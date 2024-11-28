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
}

void AGunfightPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
}

void AGunfightPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
}

void AGunfightPlayerController::SetHUDScore(float Score)
{
}

void AGunfightPlayerController::SetHUDDefeats(int32 Defeats)
{
}

void AGunfightPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
}

void AGunfightPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
}

void AGunfightPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
}

void AGunfightPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
}

void AGunfightPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
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
}

void AGunfightPlayerController::HandleMatchHasStarted()
{
}

void AGunfightPlayerController::HandleCooldown()
{
}

void AGunfightPlayerController::ServerCheckMatchState_Implementation()
{
}

void AGunfightPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
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

void AGunfightPlayerController::OnRep_MatchState()
{
}

void AGunfightPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}
