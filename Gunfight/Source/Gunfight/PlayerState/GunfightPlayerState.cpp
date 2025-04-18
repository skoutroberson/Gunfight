// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/GameInstance/GunfightGameInstanceSubsystem.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/GameMode/TeamsGameMode.h"

void AGunfightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightPlayerState, Defeats);
	DOREPLIFETIME(AGunfightPlayerState, Team);
	//DOREPLIFETIME(AGunfightPlayerState, bTeamSwapRequested);
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

		bool bColoredTeam = Team == ETeam::ET_RedTeam || Team == ETeam::ET_BlueTeam;
		if (bColoredTeam)
		{
			// Update game instance so teams match is true so if the host leaves, the game mode wont switch. TODO If I can find a better place to put this, I'll move it.
			UpdateGameInstanceTeamsMode();
		}
	}
}

void AGunfightPlayerState::RequestTeamSwap()
{
	bTeamSwapRequested = true;
	ServerRequestTeamSwap();
}

void AGunfightPlayerState::ServerRequestTeamSwap_Implementation()
{
	HandleTeamSwapRequest();
}

void AGunfightPlayerState::HandleTeamSwapRequest()
{
	ATeamsGameMode* TeamsMode = Cast<ATeamsGameMode>(UGameplayStatics::GetGameMode(this));
	if (!TeamsMode) return;
	AGunfightGameState* GState = Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this));
	if (GState == nullptr) return;
	GState->QueueOrSwapSwapper(this);
}

void AGunfightPlayerState::ClientTeamSwapped_Implementation()
{
	bTeamSwapRequested = false;
	if (GetPlayerController() == nullptr) return;
	AGunfightPlayerController* GPlayerController = Cast<AGunfightPlayerController>(GetPlayerController());
	if (GPlayerController == nullptr) return;
	GPlayerController->UpdateTeamSwapText(FString("Press and hold right stick to request a team swap."));
}

void AGunfightPlayerState::SetTeamSwapRequested(bool bNewTeamSwapRequested)
{
	if (!HasAuthority()) return;

	if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString("SetTeamSwapRequested"));

	bTeamSwapRequested = bNewTeamSwapRequested;
	//OnRep_TeamSwapRequested();
}

//void AGunfightPlayerState::OnRep_TeamSwapRequested()
//{
//	if (bTeamSwapRequested == false)
//	{
//		AGunfightPlayerController* GPlayerController = Cast<AGunfightPlayerController>(GetPlayerController());
//		if (GPlayerController == nullptr) return;
//		GPlayerController->UpdateTeamSwapText(FString("Press and hold right stick to request a team swap."));
//	}
//}

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
