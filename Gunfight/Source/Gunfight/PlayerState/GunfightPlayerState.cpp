// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/GameInstance/GunfightGameInstanceSubsystem.h"
#include "Gunfight/GameState/GunfightGameState.h"

void AGunfightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightPlayerState, Defeats);
	DOREPLIFETIME(AGunfightPlayerState, Team);
}

void AGunfightPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	OnRep_Score();
}

void AGunfightPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	OnRep_Defeats();
}

void AGunfightPlayerState::SetPlayerNameBP(const FString& NewName)
{
	bUseCustomPlayerNames = true;
	SetPlayerName(NewName);

	AGunfightPlayerController* LocalController = GetWorld()->GetFirstPlayerController<AGunfightPlayerController>();
	if (LocalController)
	{
		LocalController->UpdateScoreboard(this, EScoreboardUpdate::ESU_Death); // ESU_Death so we only update the line in the scoreboard for this player
	}
}

void AGunfightPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;

	AGunfightCharacter* GCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GCharacter)
	{
		GCharacter->SetTeamColor(Team);
	}
}

void AGunfightPlayerState::OnRep_Team()
{
	AGunfightCharacter* GCharacter = Cast<AGunfightCharacter>(GetPawn());
	if (GCharacter)
	{
		GCharacter->SetTeamColor(Team);
	}
}

void AGunfightPlayerState::RequestTeamSwap()
{
	ServerRequestTeamSwap();
	bTeamSwapRequested = true;
}

void AGunfightPlayerState::ServerRequestTeamSwap_Implementation()
{
	HandleTeamSwapRequest();
}

void AGunfightPlayerState::HandleTeamSwapRequest()
{
	if (!HasAuthority()) return;
	AGunfightGameState* GState = Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this));
	if (GState == nullptr) return;
	GState->QueueOrSwapSwapper(this);
}

void AGunfightPlayerState::ClientTeamSwapped_Implementation()
{
	bTeamSwapRequested = false;
	AGunfightPlayerController* GPlayerController = Cast<AGunfightPlayerController>(GetPlayerController());
	if (GPlayerController == nullptr) return;
	GPlayerController->UpdateTeamSwapText(FString("Press and hold right stick to request a team swap."));
}

void AGunfightPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}

		// update scoreboard
		AGunfightPlayerController* LocalController = GetWorld()->GetFirstPlayerController<AGunfightPlayerController>();
		if (LocalController)
		{
			LocalController->UpdateScoreboard(this, EScoreboardUpdate::ESU_Score);

			bool bShouldUpdateStat = (LocalController->GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress ||
				LocalController->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress ||
				LocalController->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown);

			if (!Character->IsLocallyControlled() || !bShouldUpdateStat) return;

			// update stat
			UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
			if (GameInstance == nullptr) return;
			UGunfightGameInstanceSubsystem* GunfightSubsystem = GameInstance->GetSubsystem<UGunfightGameInstanceSubsystem>();
			if (GunfightSubsystem)
			{
				GunfightSubsystem->AddToKills();
			}
		}
	}
}

void AGunfightPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
		// update scoreboard
		AGunfightPlayerController* LocalController = GetWorld()->GetFirstPlayerController<AGunfightPlayerController>();
		if (LocalController)
		{
			LocalController->UpdateScoreboard(this, EScoreboardUpdate::ESU_Death);

			bool bShouldUpdateStat = (LocalController->GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress ||
				LocalController->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress ||
				LocalController->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown);

			if (!Character->IsLocallyControlled() || !bShouldUpdateStat) return;

			// update stat

			UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
			if (GameInstance == nullptr) return;
			UGunfightGameInstanceSubsystem* GunfightSubsystem = GameInstance->GetSubsystem<UGunfightGameInstanceSubsystem>();
			if (GunfightSubsystem)
			{
				GunfightSubsystem->AddToDefeats();
			}
		}
	}
}
