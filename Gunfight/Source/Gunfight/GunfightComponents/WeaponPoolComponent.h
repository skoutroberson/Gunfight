// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponPoolComponent.generated.h"

enum class EWeaponType : uint8;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API UWeaponPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWeaponPoolComponent();

	void AddWeaponToPool(AWeapon* NewWeapon);
	void RemoveWeaponFromPool(AWeapon* WeaponToRemove);

	// Gets all weapons that are not equipped/holstered and resets them and moves them to the weapon pool location on the map outside of the playable area
	void ReclaimFieldWeapons();
	void ReclaimWeapon(AWeapon* ReclaimWeapon);

	// Gets a weapon from the pool and attaches it to the player's open holster
	AWeapon* GiveWeaponToPlayer(class AGunfightCharacter* GunfightCharacter, EWeaponType WeaponType);

	void StartFreeForAllReclaimTimer();

protected:
	virtual void BeginPlay() override;

private:
	// keeps track of all weapons in the level
	TMap<EWeaponType, TArray<AWeapon*>> WeaponMap;

	// Fills the weapon map with all of the weapons in the level
	void InitializeWeaponMap();

	FTimerHandle ReclaimWeaponTimer;
	void ReclaimWeaponTimerFinished();

	UPROPERTY(EditAnywhere)
	FVector WeaponPoolLocation = FVector(0.f, 0.f, -10000.f);

	AWeapon* GetOldestUnusedWeapon(EWeaponType WeaponType);
	
};
