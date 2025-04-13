// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gunfight/GunfightTypes/CombatState.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "Gunfight/Gunfight.h"
#include "CombatComponent.generated.h"

enum class EWeaponType : uint8;

// ps, i'm never using a bool to determine left/right again

// might have to use this if I am getting out of order OnRep bugs for these two variables
USTRUCT(BlueprintType)
struct FOwnedWeapon
{
	GENERATED_BODY()

	UPROPERTY()
	class AWeapon* EquippedWeapon;

	UPROPERTY()
	AWeapon* HolsteredWeapon;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class AGunfightCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void GrabWeapon(class AWeapon* WeaponGrabbed, bool bLeftController);

	// equip weapon in slot 1 wihle slot2 is not gripping
	void EquipWeapon(AWeapon* WeaponToEquip, bool bLeftController);
	// equip weapon in slot 2 whie slot 1 is gripping
	void EquipWeaponSecondary(AWeapon* WeaponToEquip, bool bLeftController);

	// released the hand in slot 2 while slot 1 is still gripping.
	void DropWeaponSecondary(AWeapon* WeaponToRelease, bool bLeftController);
	// released the hand in slot1 while slot2 is still gripping.
	void DropWeaponPrimary(AWeapon* WeaponToRelease, bool bLeftController);

	void DropWeapon(bool bLeftHand);
	void Reload(bool bLeft);

	void AttachWeaponToHand(bool bLeftHand);

	void AttachWeaponToHolster(AWeapon* WeaponToAttach);

	void EquipMagazine(class AFullMagazine* MagToEquip, bool bLeftController);
	void DropMagazine(bool bLeftHand);

	void FireButtonPressed(bool bPressed, bool bLeftHand);

	bool bLocallyReloading = false;

	bool bLocallyReloadingLeft = false;
	bool bLocallyReloadingRight = false;

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

	UPROPERTY()
	AWeapon* PreviousLeftEquippedWeapon;
	UPROPERTY()
	AWeapon* PreviousRightEquippedWeapon;

	void ResetWeapon(AWeapon* WeaponToReset);

	// returns false if we own no weapons
	bool ResetOwnedWeapons();

	void AddWeaponToHolster(AWeapon* WeaponToHolster, bool bLeft);

protected:
	virtual void BeginPlay() override;

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip, bool bLeftHand);
	void AttachActorToHand(AActor* ActorToAttach, bool bLeftHand, FVector RelativeOffset = FVector::ZeroVector);

	void HandleWeaponAttach(AWeapon* WeaponToAttach, bool bLeftHand);
	void AttachWeaponToMotionController(AWeapon* WeaponToAttach, bool bLeftHand);
	void AttachHandMeshToWeapon(AWeapon* Weapon, bool bLeftHand, bool bSlot1);
	// attaches hand mesh to motion controller and applys default offsets
	void HandleWeaponDrop(bool bLeftHand);
	void DetachHandMeshFromWeapon(bool bLeftHand);

	void AttachMagazineToMotionController(AFullMagazine* MagToAttach, bool bLeftHand);
	void HandleMagDrop(bool bLeftHand);

	UFUNCTION()
	void OnRep_LeftEquippedWeapon();
	UFUNCTION()
	void OnRep_RightEquippedWeapon();

	UFUNCTION()
	void OnRep_LeftHolsteredWeapon();

	UFUNCTION()
	void OnRep_RightHolsteredWeapon();

	void Fire(bool bLeft);
	void FireHitScanWeapon(bool bLeft);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void TraceUnderLeftCrosshairs(FHitResult& TraceHitResult);
	void TraceUnderRightCrosshairs(FHitResult& TraceHitResult);

	void LocalFire(const FVector_NetQuantize& TraceHitTarget, bool bLeft);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, const float FireDelay, bool bLeft);
	void ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, const float FireDelay, bool bLeft);
	bool ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay, bool bLeft);

	UFUNCTION(NetMulticast, Reliable)
	void MultiCastFire(const FVector_NetQuantize& TraceHitTarget, bool bLeft);
	void MultiCastFire_Implementation(const FVector_NetQuantize& TraceHitTarget, bool bLeft);

	UFUNCTION(Server, Reliable)
	void ServerReload(bool bLeft);
	void ServerReload_Implementation(bool bLeft);
	void HandleReload(bool bLeft);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload(bool bLeft);
	void MulticastReload_Implementation(bool bLeft);

	// returns true if we holstered the weapon
	bool HolsterWeaponDontDrop(bool bLeft, AWeapon* WeaponToDrop);

private:

	UPROPERTY();
	class AGunfightCharacter* Character;

	UPROPERTY()
	class AGunfightPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_LeftEquippedWeapon)
	AWeapon* LeftEquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_RightEquippedWeapon)
	AWeapon* RightEquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_LeftHolsteredWeapon)
	AWeapon* LeftHolsteredWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_RightHolsteredWeapon)
	AWeapon* RightHolsteredWeapon;

	UPROPERTY()
	AWeapon* PreviousLeftHolsteredWeapon;
	UPROPERTY()
	AWeapon* PreviousRightHolsteredWeapon;

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
	bool bFireButtonPressedLeft;
	bool bFireButtonPressedRight;

	bool bCanFire = true;

	bool bCanFireLeft = true;
	bool bCanFireRight = true;

	FTimerHandle FireTimer;

	FTimerHandle FireTimerLeft;
	FTimerHandle FireTimerRight;

	void StartFireTimer(float FireDelay, bool bLeft);
	void StartFireTimerLeft(float FireDelay);
	void StartFireTimerRight(float FireDelay);

	void FireTimerFinished();
	void FireTimerFinishedLeft();
	void FireTimerFinishedRight();

	bool CanFire(bool bLeft);

	FVector HitTarget;

	FVector HitTargetLeft;
	FVector HitTargetRight;

	void UpdateWeaponAmmos(bool bLeft);
	void UpdateCarriedAmmo();
	void UpdateAmmoValues();
	int32 AmountToReload(AWeapon *Weapon);

	EHandState SlotToHandState(EWeaponType WeaponType, bool bSlot1);

	// two hand
	void RotateWeaponTwoHand(float DeltaTime);

	ESide GetClosestHolster(FVector WeaponLocation, float &OutDistance);
	void AttachWeaponToHolster(AWeapon* WeaponToHolster, ESide HolsterSide);

	FName SideToHolsterName(ESide HolsterSide);

	// used in Tick if we are holding a weapon with two hands, will release the players grip if their hand is too far
	void CheckSecondHand(class UMotionControllerComponent* MC);

public:	
	AWeapon* GetEquippedWeapon(bool bLeft);
	// returns true if the corresponding Weapon*: LeftEquippedWeapon or RightEquipWeapoin is not nullptr
	bool CheckEquippedWeapon(bool bLeft);
	// returns true if the corresponding FullMagazine*: LeftEquippedMagazine or RightEquippedMagazine is not nullptr
	bool CheckEquippedMagazine(bool bLeft);
	AFullMagazine* GetEquippedMagazine(bool bLeft);
	void SetEquippedMagazine(AFullMagazine* NewMag, bool bLeft);
	bool IsWeaponEquipped();
	AWeapon* GetHolsteredWeapon(bool bLeft);
};
