// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gunfight/GunfightTypes/CombatState.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "Gunfight/Gunfight.h"
#include "CombatComponent.generated.h"

// ps, i'm never using a bool to determine left/right again

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class AGunfightCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip, bool bLeftController);
	void DropWeapon(bool bLeftHand);
	void Reload();

	void AttachWeaponToHand(bool bLeftHand);

	void AttachWeaponToHolster(AWeapon* WeaponToAttach);

	void EquipMagazine(class AFullMagazine* MagToEquip, bool bLeftController);
	void DropMagazine(bool bLeftHand);

	void FireButtonPressed(bool bPressed, bool bLeftHand);

	bool bLocallyReloading = false;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttachToHolster(ESide HandSide);
	void MulticastAttachToHolster_Implementation(ESide HandSide);

	UFUNCTION(Server, Reliable)
	void ServerDropWeapon(
		bool bLeft, 
		FVector_NetQuantize StartLocation, 
		FRotator StartRotation,
		FVector_NetQuantize LinearVelocity,
		FVector_NetQuantize AngularVelocity
	);
	void ServerDropWeapon_Implementation(
		bool bLeft,
		FVector_NetQuantize StartLocation,
		FRotator StartRotation,
		FVector_NetQuantize LinearVelocity,
		FVector_NetQuantize AngularVelocity
	);

protected:
	virtual void BeginPlay() override;

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip, bool bLeftHand);
	void AttachActorToHand(AActor* ActorToAttach, bool bLeftHand, FVector RelativeOffset = FVector::ZeroVector);

	void HandleWeaponAttach(AWeapon* WeaponToAttach, bool bLeftHand);
	void AttachWeaponToMotionController(AWeapon* WeaponToAttach, bool bLeftHand);
	// attaches hand mesh to motion controller and applys default offsets
	void HandleWeaponDrop(bool bLeftHand);

	void AttachMagazineToMotionController(AFullMagazine* MagToAttach, bool bLeftHand);
	void HandleMagDrop(bool bLeftHand);

	UFUNCTION()
	void OnRep_LeftEquippedWeapon();
	UFUNCTION()
	void OnRep_RightEquippedWeapon();

	void Fire(bool bLeft);
	void FireHitScanWeapon();

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, const float FireDelay);
	void ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, const float FireDelay);
	bool ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastFire(const FVector_NetQuantize& TraceHitTarget);
	void MultiCastFire_Implementation(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable)
	void ServerReload();
	void ServerReload_Implementation();
	void HandleReload();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload();
	void MulticastReload_Implementation();

	// returns true if we holstered the weapon
	bool HolsterWeaponDontDrop(bool bLeft, AWeapon* WeaponToDrop, FVector HolsterLocation);

private:

	UPROPERTY();
	class AGunfightCharacter* Character;

	UPROPERTY()
	class AGunfightPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_LeftEquippedWeapon)
	AWeapon* LeftEquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_RightEquippedWeapon)
	AWeapon* RightEquippedWeapon;

	UPROPERTY()
	AFullMagazine* LeftEquippedMagazine;
	UPROPERTY()
	AFullMagazine* RightEquippedMagazine;

	UPROPERTY(EditAnywhere)
	float GunPickupDistance = 100.f;

	bool CanPickupGun(bool bLeft);

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void UpdateWeaponStateOnPickup(AWeapon* WeaponPickedUp);

	UPROPERTY(ReplicatedUsing = OnRep_CombatState, VisibleAnywhere)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	bool bFireButtonPressed;

	bool bCanFire = true;

	FTimerHandle FireTimer;

	void StartFireTimer(float FireDelay);
	void FireTimerFinished();

	bool CanFire(bool bLeft);

	FVector HitTarget;

	void UpdateWeaponAmmos();
	void UpdateCarriedAmmo();
	void UpdateAmmoValues();
	int32 AmountToReload();

public:	
	AWeapon* GetEquippedWeapon(bool bLeft);
	// returns true if the corresponding Weapon*: LeftEquippedWeapon or RightEquipWeapoin is not nullptr
	bool CheckEquippedWeapon(bool bLeft);
	// returns true if the corresponding FullMagazine*: LeftEquippedMagazine or RightEquippedMagazine is not nullptr
	bool CheckEquippedMagazine(bool bLeft);
	AFullMagazine* GetEquippedMagazine(bool bLeft);
	void SetEquippedMagazine(AFullMagazine* NewMag, bool bLeft);
	bool IsWeaponEquipped();
};
