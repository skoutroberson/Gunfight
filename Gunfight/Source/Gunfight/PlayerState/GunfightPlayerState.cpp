// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"

void AGunfightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightPlayerState, Defeats);
	//DOREPLIFETIME(AGunfightPlayerState, Team);
}

void AGunfightPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
		AGunfightPlayerController* LocalController = GetWorld()->GetFirstPlayerController<AGunfightPlayerController>();
		if (LocalController)
		{
			LocalController->UpdateScoreboard(this, EScoreboardUpdate::ESU_Score);
		}
	}
}

void AGunfightPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
		AGunfightPlayerController* LocalController = GetWorld()->GetFirstPlayerController<AGunfightPlayerController>();
		if (LocalController)
		{
			LocalController->UpdateScoreboard(this, EScoreboardUpdate::ESU_Death);
		}
	}
}

void AGunfightPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetPawn()) : Character;
	if (Character)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, FString::Printf(TEXT("OnRep_Score")));
		}
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
	}
}