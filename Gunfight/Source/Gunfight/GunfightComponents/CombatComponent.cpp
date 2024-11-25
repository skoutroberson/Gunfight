// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/Weapon/Weapon.h"
#include "GripMotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, LeftEquippedWeapon);
	DOREPLIFETIME(UCombatComponent, RightEquippedWeapon);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip, bool bLeftController)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (bLeftController && LeftEquippedWeapon == nullptr)
	{
		EquipPrimaryWeapon(WeaponToEquip, true);
	}
	else if(!bLeftController && RightEquippedWeapon == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Equip Right Weapon"));
		EquipPrimaryWeapon(WeaponToEquip, false);
	}
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip, bool bLeftHand)
{
	if (WeaponToEquip == nullptr || Character == nullptr) return;
	/*
	AWeapon& EquippedWeapon = bLeftHand ? *LeftEquippedWeapon : *RightEquippedWeapon;
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetCharacterOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	*/
	if (bLeftHand)
	{
		LeftEquippedWeapon = WeaponToEquip;
		LeftEquippedWeapon->SetOwner(Character);
		LeftEquippedWeapon->SetCharacterOwner(Character);
		LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToHand(LeftEquippedWeapon, bLeftHand);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipPrimaryWeapon"));
		RightEquippedWeapon = WeaponToEquip;
		RightEquippedWeapon->SetOwner(Character);
		RightEquippedWeapon->SetCharacterOwner(Character);
		RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToHand(RightEquippedWeapon, bLeftHand);
	}
	Character->SetHandState(bLeftHand, EHandState::EHS_HoldingPistol);
	// TODO
	/*
		EquippedWeapon->SetHUDAmmo();
		PlayEquipWeaponSound(WeaponToEquip);
		WeaponToEquip->ClientUpdateAmmoOnPickup(WeaponToEquip->GetAmmo());
		ReloadEmptyWeapon();
	*/
}

void UCombatComponent::AttachActorToHand(AActor* ActorToAttach, bool bLeftHand)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const FName SocketName = bLeftHand ? FName("LeftHandWeapon") : FName("RightHandWeapon");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
	//Character->SetHandState(bLeftHand, EHandState::EHS_HoldingPistol);
}

void UCombatComponent::OnRep_LeftEquippedWeapon()
{
	if (LeftEquippedWeapon && Character)
	{
		LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToHand(LeftEquippedWeapon, true);
		PlayEquipWeaponSound(LeftEquippedWeapon);
		Character->SetHandState(true, EHandState::EHS_HoldingPistol);
		//LeftEquippedWeapon->SetHUDAmmo();
		return;
	}
	Character->SetHandState(true, EHandState::EHS_Idle);
}

void UCombatComponent::OnRep_RightEquippedWeapon()
{
	if (RightEquippedWeapon && Character)
	{
		RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToHand(RightEquippedWeapon, false);
		PlayEquipWeaponSound(RightEquippedWeapon);
		Character->SetHandState(false, EHandState::EHS_HoldingPistol);
		//RightEquippedWeapon->SetHUDAmmo();
		return;
	}
	Character->SetHandState(false, EHandState::EHS_Idle);
}

void UCombatComponent::DropWeapon(bool bLeftHand)
{
	if (bLeftHand && LeftEquippedWeapon)
	{
		LeftEquippedWeapon->Dropped(true);
		LeftEquippedWeapon = nullptr;
	}
	else if (!bLeftHand && RightEquippedWeapon)
	{
		RightEquippedWeapon->Dropped(false);
		RightEquippedWeapon = nullptr;
	}
}

void UCombatComponent::AttachWeaponToHolster(AWeapon* WeaponToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr) return;
	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HolsterSocket)
	{
		HolsterSocket->AttachActor(WeaponToAttach, Character->GetMesh());
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{

}

void UCombatComponent::UpdateWeaponStateOnPickup(AWeapon* WeaponPickedUp)
{
	if (WeaponPickedUp->GetAmmo() > 0)
	{
		WeaponPickedUp->SetWeaponState(EWeaponState::EWS_Ready);
	}
	else
	{
		WeaponPickedUp->SetWeaponState(EWeaponState::EWS_Empty);
	}
}

void UCombatComponent::OnRep_CombatState()
{

}

AWeapon* UCombatComponent::GetEquippedWeapon(bool bLeft)
{
	return bLeft ? LeftEquippedWeapon : RightEquippedWeapon;
}
