// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponPoolComponent.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/Weapon/FullMagazine.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"

UWeaponPoolComponent::UWeaponPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UWeaponPoolComponent::ReclaimFieldWeapons()
{
	for (auto& MapPair : WeaponMap)
	{
		for (auto Weapon : MapPair.Value)
		{
			if (Weapon == nullptr) continue;
			if (Weapon->GetOwner() == nullptr)
			{
				ReclaimWeapon(Weapon);
			}
		}
	}
}

void UWeaponPoolComponent::ReclaimWeapon(AWeapon* ReclaimWeapon)
{
	if (ReclaimWeapon == nullptr) return;
	ReclaimWeapon->bAlwaysRelevant = false;

	ReclaimWeapon->SetActorRelativeLocation(FVector::ZeroVector);
	ReclaimWeapon->SetActorRelativeRotation(FRotator::ZeroRotator);
	ReclaimWeapon->SetActorLocationAndRotation(WeaponPoolLocation, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);

	if (ReclaimWeapon->GetMagazine())
	{
		ReclaimWeapon->GetMagazine()->Destroy();
		ReclaimWeapon->UnhideMag();
	}

	ReclaimWeapon->SetAmmo(ReclaimWeapon->GetMagCapacity());
	ReclaimWeapon->SetCarriedAmmo(ReclaimWeapon->GetMaxCarriedAmmo());

	USkeletalMeshComponent* WeaponMesh = ReclaimWeapon->GetWeaponMesh();
	if (WeaponMesh == nullptr) return;

	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetVisibility(false);
}

AWeapon* UWeaponPoolComponent::GiveWeaponToPlayer(AGunfightCharacter* GunfightCharacter, EWeaponType WeaponType)
{
	AWeapon* WeaponToGive = GetOldestUnusedWeapon(WeaponType);
	if (WeaponToGive == nullptr || WeaponToGive->GetWeaponMesh() == nullptr || GunfightCharacter == nullptr || GunfightCharacter->GetCombat() == nullptr) return nullptr;

	WeaponToGive->bAlwaysRelevant = true;
	WeaponToGive->GetWeaponMesh()->SetVisibility(true);
	GunfightCharacter->GetCombat()->AddWeaponToHolster(WeaponToGive, false);
	GunfightCharacter->GetCombat()->ResetWeapon(WeaponToGive);
	GunfightCharacter->SetDefaultWeapon(WeaponToGive);

	return WeaponToGive;
}

void UWeaponPoolComponent::StartFreeForAllReclaimTimer()
{
	if (GetOwner() == nullptr) return;
	GetOwner()->GetWorldTimerManager().SetTimer(ReclaimWeaponTimer, this, &UWeaponPoolComponent::ReclaimWeaponTimerFinished, 5.f, true);
}

void UWeaponPoolComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeWeaponMap();
}

void UWeaponPoolComponent::AddWeaponToPool(AWeapon* NewWeapon)
{
	if (NewWeapon == nullptr) return;

	WeaponMap.FindOrAdd(NewWeapon->GetWeaponType()).AddUnique(NewWeapon);
}

void UWeaponPoolComponent::RemoveWeaponFromPool(AWeapon* WeaponToRemove)
{
	if (WeaponToRemove == nullptr) return;

	TArray<AWeapon*>* WeaponArray = WeaponMap.Find(WeaponToRemove->GetWeaponType());
	if (WeaponArray != nullptr)
	{
		WeaponArray->RemoveSingle(WeaponToRemove);
	}
}

void UWeaponPoolComponent::InitializeWeaponMap()
{
	// fill weapon map
	TArray<AActor*> WeaponActors;
	UGameplayStatics::GetAllActorsOfClass(this, AWeapon::StaticClass(), WeaponActors);

	for (auto w : WeaponActors)
	{
		AWeapon* Weapon = Cast<AWeapon>(w);
		if (Weapon)
		{
			WeaponMap.FindOrAdd(Weapon->GetWeaponType()).AddUnique(Weapon);
		}
	}
}

void UWeaponPoolComponent::ReclaimWeaponTimerFinished()
{
	// loop through all weapons, reclaiming dropped weapons last used over 30 seconds ago.
	// Reclaim

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	for (auto& MapPair : WeaponMap)
	{
		for (auto Weapon : MapPair.Value)
		{
			if (Weapon == nullptr) continue;
			if (Weapon->GetOwner() == nullptr && (World->GetTimeSeconds() - Weapon->GetLastTimeInteractedWith()) > 10.f)
			{
				ReclaimWeapon(Weapon);
			}
		}
	}
}

AWeapon* UWeaponPoolComponent::GetOldestUnusedWeapon(EWeaponType WeaponType)
{
	UWorld* World = GetWorld();
	TArray<AWeapon*>& WeaponArray = WeaponMap.FindOrAdd(WeaponType);
	if (WeaponArray.IsEmpty() || World == nullptr) return nullptr;

	AWeapon* OldestUnusedWeapon = nullptr;
	float OldestWeaponTime = -WEAPON_START_TIME;

	for (auto w : WeaponArray)
	{
		if (w->GetOwner() == nullptr)
		{
			float TimeDif = World->GetTimeSeconds() - w->GetLastTimeInteractedWith();
			if (TimeDif > OldestWeaponTime)
			{
				OldestUnusedWeapon = w;
				OldestWeaponTime = w->GetLastTimeInteractedWith();
			}
		}
	}

	return OldestUnusedWeapon;
}
