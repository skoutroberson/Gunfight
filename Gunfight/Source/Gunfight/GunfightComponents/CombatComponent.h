// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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


	void AttachWeaponToHand(bool bLeftHand);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_EquipWeapon(AWeapon* WeaponToEquip, bool bLeft);
	void Multicast_EquipWeapon_Implementation(AWeapon* WeaponToEquip, bool bLeft);

private:

	UPROPERTY();
	class AGunfightCharacter* Character;
	UPROPERTY();
	class AWeapon* CharacterWeapon;

	UPROPERTY(EditAnywhere)
	float GunPickupDistance = 100.f;

	bool CanPickupGun(bool bLeft);

	UPROPERTY()
	AWeapon* EquippedWeapon;

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void UpdateWeaponStateOnPickup(AWeapon* WeaponPickedUp);

public:	
	
};
