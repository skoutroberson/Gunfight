// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/GameInstance/GunfightGameInstanceSubsystem.h"

void AGunfightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightPlayerState, Defeats);
	//DOREPLIFETIME(AGunfightPlayerState, Team);
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
		}

		if (!Character->IsLocallyControlled()) return;

		// update save game
		UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
		if (GameInstance == nullptr) return;
		UGunfightGameInstanceSubsystem* GunfightSubsystem = GameInstance->GetSubsystem<UGunfightGameInstanceSubsystem>();
		if (GunfightSubsystem)
		{
			GunfightSubsystem->AddToKills();
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
		}

		if (!Character->IsLocallyControlled()) return;

		UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
		if (GameInstance == nullptr) return;
		UGunfightGameInstanceSubsystem* GunfightSubsystem = GameInstance->GetSubsystem<UGunfightGameInstanceSubsystem>();
		if (GunfightSubsystem)
		{
			GunfightSubsystem->AddToDefeats();
		}
	}
}
