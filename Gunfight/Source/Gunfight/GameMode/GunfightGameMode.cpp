// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameMode.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/WeaponPoolComponent.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "Algo/RandomShuffle.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AGunfightGameMode::AGunfightGameMode()
{
	bDelayedStart = true;
	GunfightMatchState = EGunfightMatchState::EGMS_Uninitialized;

	WeaponPool = CreateDefaultSubobject<UWeaponPoolComponent>(TEXT("WeaponPool"));
}

void AGunfightGameMode::GiveMeWeapon(AGunfightCharacter* Char, EWeaponType WeaponType)
{
	if (WeaponPool == nullptr) return;

	WeaponPool->GiveWeaponToPlayer(Char, WeaponType);
}

void AGunfightGameMode::AddWeaponToPool(AWeapon* NewWeapon)
{
	if (WeaponPool == nullptr || NewWeapon == nullptr) return;
	WeaponPool->AddWeaponToPool(NewWeapon);
}

void AGunfightGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();

	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Spawnpoints);

	if (bTeamsMatch)
	{
		UGameplayStatics::GetAllActorsOfClassWithTag(this, APlayerStart::StaticClass(), FName("TeamA"), TeamASpawns);
		UGameplayStatics::GetAllActorsOfClassWithTag(this, APlayerStart::StaticClass(), FName("TeamB"), TeamBSpawns);
	}

	SetGunfightMatchState(EGunfightMatchState::EGMS_WaitingForPlayers);
}

void AGunfightGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WaitingToStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
			SetGunfightMatchState(EGunfightMatchState::EGMS_WaitingForPlayers);
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		TickGunfightMatchState(DeltaTime);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WaitingToStartTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AGunfightGameMode::TickGunfightMatchState(float DeltaTime)
{
	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup)
	{
		CountdownTime = GunfightWarmupTime + WaitingToStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartGunfightMatch();
		}
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress)
	{
		CountdownTime = GunfightMatchTime + GunfightWarmupTime + WaitingToStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			EndGunfightMatch();
		}
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown)
	{
		CountdownTime = GunfightCooldownTime + GunfightMatchTime + GunfightWarmupTime + WaitingToStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGunfightMatch();
		}
	}
}

void AGunfightGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
}

void AGunfightGameMode::HandleGunfightWarmupStarted()
{
	if (WeaponPool)
	{
		WeaponPool->StartFreeForAllReclaimTimer(); // reclaim all weapons that haven't been used for over 30 seconds.
	}
}

void AGunfightGameMode::HandleGunfightMatchStarted()
{
}

void AGunfightGameMode::HandleGunfightMatchEnded()
{
	
}

void AGunfightGameMode::StartGunfightWarmup()
{
	GunfightMatchState = EGunfightMatchState::EGMS_Warmup;
}

void AGunfightGameMode::StartGunfightMatch()
{
	// spawn all players at random spawn points, allow combat

	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	if (GunfightGameState)
	{
		GunfightGameState->TopScoringPlayers.Empty();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			AGunfightCharacter* GunfightPlayerCharacter = Cast<AGunfightCharacter>(GunfightPlayer->GetPawn());
			AGunfightPlayerState* GunfightPlayerState = Cast<AGunfightPlayerState>(GunfightPlayer->PlayerState);
			if (GunfightPlayerState && GunfightPlayerCharacter)
			{
				GunfightPlayerState->SetDefeats(0);
				GunfightPlayerState->SetScore(0.f);
				RequestRespawn(GunfightPlayerCharacter, GunfightPlayer);
				//GunfightPlayer->ClientUpdateMatchState(EGunfightMatchState::EGMS_MatchInProgress);
			}
		}
	}

	SetGunfightMatchState(EGunfightMatchState::EGMS_MatchInProgress);
}

void AGunfightGameMode::EndGunfightMatch()
{
	// show scoreboard and play announcement for Win/Loss

	SetGunfightMatchState(EGunfightMatchState::EGMS_MatchCooldown);
}

void AGunfightGameMode::RestartGunfightMatch()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->GetTimeSeconds();

	if (bTeamsMatch)
	{
		SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_Warmup);
	}
	else
	{
		SetGunfightMatchState(EGunfightMatchState::EGMS_Warmup);
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString("RestartGunfightMatch"));

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			//GunfightPlayer->ClientRestartGame(EGunfightMatchState::EGMS_Warmup, GunfightWarmupTime, GunfightMatchTime, GunfightCooldownTime, 0.f);

			AGunfightPlayerState* GunfightPlayerState = GunfightPlayer->GetPlayerState<AGunfightPlayerState>();
			if (GunfightPlayerState)
			{
				GunfightPlayerState->SetScore(0.f);
				GunfightPlayerState->SetDefeats(0);
			}
		}
	}
}

void AGunfightGameMode::SetGunfightMatchState(EGunfightMatchState NewState)
{
	if (GunfightMatchState == NewState)
	{
		return;
	}

	GunfightMatchState = NewState;

	OnGunfightMatchStateSet();

	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	if (GunfightGameState)
	{
		GunfightGameState->SetGunfightMatchState(NewState);
	}
}

void AGunfightGameMode::OnGunfightMatchStateSet()
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
		HandleGunfightMatchEnded();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			GunfightPlayer->SetGunfightMatchState(GunfightMatchState);
		}
	}
}

void AGunfightGameMode::ShuffleTeamSpawns()
{
	Algo::RandomShuffle(TeamASpawns);
	Algo::RandomShuffle(TeamBSpawns);
}

AActor* AGunfightGameMode::GetBestSpawnpoint(AController* ElimmedController)
{
	if (bTeamsMatch)
	{
		AGunfightPlayerState* GPState = ElimmedController->GetPlayerState<AGunfightPlayerState>();
		if (GPState == nullptr || GPState->GetTeam() == ETeam::ET_NoTeam) return nullptr;

		if (GPState->GetTeam() == ETeam::ET_RedTeam)
		{
			Spawnpoints = bRedSpawnsA ? TeamASpawns : TeamBSpawns;
		}
		else
		{
			Spawnpoints = bRedSpawnsA ? TeamBSpawns : TeamASpawns;
		}
	}
	if (Spawnpoints.IsEmpty()) return nullptr;

	AActor* BestSpawn = Spawnpoints[0];
	float ClosestPlayerDistance = 0.f;

	AGunfightGameState* GState = Cast<AGunfightGameState>(GameState);
	if (GState == nullptr) return nullptr;

	const TArray<TObjectPtr<APlayerState>>& PlayerArray = GState->PlayerArray;

	if (PlayerArray.IsEmpty()) return nullptr;

	for (AActor* Spawn : Spawnpoints)
	{
		if (Spawn == nullptr) continue;
		float ClosestDistance = FLT_MAX;
		for (auto PState : PlayerArray)
		{
			APawn* CurrentPawn = PState->GetPawn();
			if (CurrentPawn == nullptr) continue;
			float CurrentDistance = FVector::DistSquared2D(Spawn->GetActorLocation(), CurrentPawn->GetActorLocation());
			if (CurrentDistance < ClosestDistance)
			{
				ClosestDistance = CurrentDistance;
			}
		}

		if (ClosestDistance > ClosestPlayerDistance)
		{
			ClosestPlayerDistance = ClosestDistance;
			BestSpawn = Spawn;
		}
	}

	return BestSpawn;
}

void AGunfightGameMode::PlayerEliminated(AGunfightCharacter* ElimmedCharacter, AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	AGunfightPlayerState* AttackerPlayerState = AttackerController ? Cast<AGunfightPlayerState>(AttackerController->PlayerState) : nullptr;
	AGunfightPlayerState* VictimPlayerState = VictimController ? Cast<AGunfightPlayerState>(VictimController->PlayerState) : nullptr;

	GunfightGameState = GetGameState<AGunfightGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && GunfightGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		GunfightGameState->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}
}

void AGunfightGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController, bool bInputDisabled)
{
	AActor* BestSpawnpoint = GetBestSpawnpoint(ElimmedController);
	if (BestSpawnpoint == nullptr) return;
	FVector SpawnLocation = BestSpawnpoint->GetActorLocation();
	FRotator SpawnRotation = BestSpawnpoint->GetActorRotation();

	APlayerStart* PlayerStart = Cast<APlayerStart>(BestSpawnpoint);
	if (PlayerStart && PlayerStart->GetCapsuleComponent())
	{
		SpawnLocation.Z -= PlayerStart->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	if (ElimmedController)
	{
		//RestartPlayerAtPlayerStart(ElimmedController, BestSpawnpoint);
	}
	if (ElimmedCharacter)
	{
		/**
		* TODO: Don't destroy, just reset HUD/Ammo/Health/Weapon/Input and spawn at the best spawnpoint
		*/
		//ElimmedCharacter->Reset();
		//ElimmedCharacter->Destroy();
		AGunfightCharacter* GCharacter = Cast<AGunfightCharacter>(ElimmedCharacter);
		if (GCharacter)
		{
			GCharacter->MulticastRespawn(SpawnLocation, SpawnRotation, bInputDisabled);
		}
	}
}

void AGunfightGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	AGunfightPlayerState* NewPlayerState = NewPlayer->GetPlayerState<AGunfightPlayerState>();
	AGunfightPlayerController* GunfightPlayerController = Cast<AGunfightPlayerController>(NewPlayer);

	if (GunfightGameState && NewPlayerState)
	{
		GunfightGameState->SortedPlayers.AddUnique(NewPlayerState);
		GunfightGameState->OnRep_SortedPlayers();
	}
	if (GunfightPlayerController)
	{
		// call client RPC on GunfightPlayerController to setup the HUD
		GunfightPlayerController->SetGunfightMatchState(GunfightMatchState);
		//GunfightPlayerController->ClientPostLoginSetMatchState(GunfightMatchState);
	}

	if (!bTeamsMatch)
	{
		int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		if (GunfightMatchState == EGunfightMatchState::EGMS_WaitingForPlayers && NumberOfPlayers > 1)
		{
			RestartGunfightMatch();
		}
	}
}

void AGunfightGameMode::Logout(AController* Exiting)
{
	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;

	AGunfightPlayerState* ExitingPlayerState = Exiting->GetPlayerState<AGunfightPlayerState>();

	if (GunfightGameState && ExitingPlayerState)
	{
		GunfightGameState->SortedPlayers.RemoveSingle(ExitingPlayerState);
		GunfightGameState->OnRep_SortedPlayers();
		GunfightGameState->PlayerArray.RemoveSingle(ExitingPlayerState);
	}

	Super::Logout(Exiting);
}

void AGunfightGameMode::PlayerLeftGame(AGunfightPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	if (GunfightGameState && GunfightGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		GunfightGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
}

void AGunfightGameMode::DestroyWeapon(AWeapon* WeaponToDestroy)
{
	if (WeaponToDestroy == nullptr) return;
	if (WeaponPool)
	{
		WeaponPool->RemoveWeaponFromPool(WeaponToDestroy);
	}
	WeaponToDestroy->Destroy();
}

float AGunfightGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}
