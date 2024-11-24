// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gunfight/GunfightTypes/CombatState.h"
#include "CombatComponent.generated.h"


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

	void AttachWeaponToHand(bool bLeftHand);

	void AttachWeaponToHolster(AWeapon* WeaponToAttach);



protected:
	virtual void BeginPlay() override;

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip, bool bLeftHand);
	void AttachActorToHand(AActor* ActorToAttach, bool bLeftHand);

	UFUNCTION()
	void OnRep_LeftEquippedWeapon();
	UFUNCTION()
	void OnRep_RightEquippedWeapon();

private:

	UPROPERTY();
	class AGunfightCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_LeftEquippedWeapon)
	AWeapon* LeftEquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_RightEquippedWeapon)
	AWeapon* RightEquippedWeapon;

	UPROPERTY(EditAnywhere)
	float GunPickupDistance = 100.f;

	bool CanPickupGun(bool bLeft);

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void UpdateWeaponStateOnPickup(AWeapon* WeaponPickedUp);

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

public:	
	FORCEINLINE AWeapon* GetEquippedWeapon(bool bLeft);
};
