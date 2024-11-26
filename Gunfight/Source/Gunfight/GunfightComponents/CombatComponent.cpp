// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/Weapon/Weapon.h"
#include "GripMotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Gunfight/Weapon/FullMagazine.h"

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
	if (!Character->GetOverlappingWeapon() || !Character->GetOverlappingWeapon()->CheckHandOverlap(bLeftController) || CheckEquippedWeapon(!bLeftController)) return;

	if (bLeftController && LeftEquippedWeapon == nullptr)
	{
		EquipPrimaryWeapon(WeaponToEquip, true);
	}
	else if(!bLeftController && RightEquippedWeapon == nullptr)
	{
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
	UE_LOG(LogTemp, Warning, TEXT("Equip Primary Weapon"));
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

void UCombatComponent::EquipMagazine(AFullMagazine* MagToEquip, bool bLeftController)
{
	if (bLeftController && !LeftEquippedMagazine)
	{
		LeftEquippedMagazine = MagToEquip;

	}
	else if (!bLeftController && !RightEquippedMagazine)
	{
		RightEquippedMagazine = MagToEquip;
	}

	MagToEquip->Equipped();
	AttachActorToHand(MagToEquip, bLeftController, MagToEquip->GetHandSocketOffset());
	
	// add magazine offset too i think
}

void UCombatComponent::DropMagazine(bool bLeftHand)
{
	if (bLeftHand && LeftEquippedMagazine)
	{
		LeftEquippedMagazine->Dropped();
		LeftEquippedMagazine = nullptr;
	}
	else if(!bLeftHand && RightEquippedMagazine)
	{
		RightEquippedMagazine->Dropped();
		RightEquippedMagazine = nullptr;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed, bool bLeftHand)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire(bLeftHand);
	}
}

void UCombatComponent::AttachActorToHand(AActor* ActorToAttach, bool bLeftHand, FVector RelativeOffset)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const FName SocketName = bLeftHand ? FName("LeftHandWeapon") : FName("RightHandWeapon");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
		ActorToAttach->SetActorRelativeLocation(RelativeOffset);
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

void UCombatComponent::Fire(bool bLeft)
{
	if (CanFire(bLeft))
	{
		const AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
		if (CurrentWeapon)
		{
			bCanFire = false;

			switch (CurrentWeapon->FireType)
			{
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			}
			StartFireTimer(CurrentWeapon->FireDelay);
		}

	}
}

void UCombatComponent::FireHitScanWeapon()
{

}

void UCombatComponent::StartFireTimer(float FireDelay)
{
	if (Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && IsWeaponEquipped() && Character->GetDefaultWeapon()->bAutomatic)
	{
		if (GetEquippedWeapon(true))
		{
			Fire(true);
		}
		else if (GetEquippedWeapon(false))
		{
			Fire(false);
		}
	}
	if (Character->GetDefaultWeapon()->IsEmpty())
	{
		//Character->GetDefaultWeapon()->PlayReload
	}
}

bool UCombatComponent::CanFire(bool bLeft)
{
	const AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
	if (CurrentWeapon == nullptr) return false;
	if (bLocallyReloading) return false;
	return !CurrentWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
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

bool UCombatComponent::CheckEquippedWeapon(bool bLeft)
{
	if (bLeft && LeftEquippedWeapon)
	{
		return true;
	}
	else if (!bLeft && RightEquippedWeapon)
	{
		return true;
	}
	return false;
}

bool UCombatComponent::CheckEquippedMagazine(bool bLeft)
{
	if (bLeft && LeftEquippedMagazine)
	{
		return true;
	}
	else if(!bLeft && RightEquippedMagazine)
	{
		return true;
	}
	return false;
}

AFullMagazine* UCombatComponent::GetEquippedMagazine(bool bLeft)
{
	return bLeft ? LeftEquippedMagazine : RightEquippedMagazine;
}

void UCombatComponent::SetEquippedMagazine(AFullMagazine* NewMag, bool bLeft)
{
	if (bLeft)
	{
		LeftEquippedMagazine = NewMag;
	}
	else
	{
		RightEquippedMagazine = NewMag;
	}
}

bool UCombatComponent::IsWeaponEquipped()
{
	if (LeftEquippedWeapon)
	{
		return true;
	}
	if (RightEquippedWeapon)
	{
		return true;
	}
	return false;
}
