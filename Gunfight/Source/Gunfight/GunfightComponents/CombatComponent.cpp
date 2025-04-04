// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "MotionControllerComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/Weapon/Weapon.h"
#include "GripMotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Gunfight/Weapon/FullMagazine.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/Gunfight.h"
#include "Components/SphereComponent.h"

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

	if (Character && Character->IsLocallyControlled() && Character->GetDefaultWeapon())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, LeftEquippedWeapon);
	DOREPLIFETIME(UCombatComponent, RightEquippedWeapon);
	DOREPLIFETIME(UCombatComponent, CombatState);
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
	
	if (bLeftHand)
	{
		LeftEquippedWeapon = WeaponToEquip;
		LeftEquippedWeapon->SetOwner(Character);
		LeftEquippedWeapon->SetCharacterOwner(Character);
		LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		LeftEquippedWeapon->SetWeaponSide(ESide::ES_Left);
		//AttachActorToHand(LeftEquippedWeapon, bLeftHand);
	}
	else
	{
		RightEquippedWeapon = WeaponToEquip;
		RightEquippedWeapon->SetOwner(Character);
		RightEquippedWeapon->SetCharacterOwner(Character);
		RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		RightEquippedWeapon->SetWeaponSide(ESide::ES_Right);
		//AttachActorToHand(RightEquippedWeapon, bLeftHand);
	}
	Character->SetHandState(bLeftHand, EHandState::EHS_HoldingPistol);
	HandleWeaponAttach(WeaponToEquip, bLeftHand);
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
	if (bLeftController)
	{
		LeftEquippedMagazine = MagToEquip;

	}
	else if (!bLeftController)
	{
		RightEquippedMagazine = MagToEquip;
	}

	MagToEquip->Equipped();
	AttachMagazineToMotionController(MagToEquip, bLeftController);
	//AttachActorToHand(MagToEquip, bLeftController, MagToEquip->GetHandSocketOffset());
	
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

void UCombatComponent::HandleWeaponAttach(AWeapon* WeaponToAttach, bool bLeftHand)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || WeaponToAttach == nullptr) return;

	if (Character->IsLocallyControlled())
	{
		AttachWeaponToMotionController(WeaponToAttach, bLeftHand);
	}
	else
	{
		AttachActorToHand(WeaponToAttach, bLeftHand);
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

void UCombatComponent::AttachWeaponToMotionController(AWeapon* WeaponToAttach, bool bLeftHand)
{
	if (Character == nullptr || WeaponToAttach == nullptr) return;
	UGripMotionControllerComponent* MotionController = bLeftHand ? Character->LeftMotionController.Get() : Character->RightMotionController.Get();
	if (MotionController == nullptr) return;

	USkeletalMeshComponent* HandMesh;
	FVector LocationOffset;
	FRotator RotationOffset;

	WeaponToAttach->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	WeaponToAttach->SetActorRelativeTransform(WeaponToAttach->GrabOffset);

	// hands

	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		LocationOffset = WeaponToAttach->HandOffsetLeft.GetLocation();
		RotationOffset = WeaponToAttach->HandOffsetLeft.GetRotation().Rotator();
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		LocationOffset = WeaponToAttach->HandOffsetRight.GetLocation();
		RotationOffset = WeaponToAttach->HandOffsetRight.GetRotation().Rotator();
	}

	if (HandMesh == nullptr) return;

	HandMesh->AttachToComponent(WeaponToAttach->GetWeaponMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HandMesh->SetRelativeLocationAndRotation(LocationOffset, RotationOffset);
}

void UCombatComponent::HandleWeaponDrop(bool bLeftHand)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || !Character->IsLocallyControlled()) return;
	USkeletalMeshComponent* HandMesh;
	UMotionControllerComponent* MotionController;
	FVector LocationOffset;
	FRotator RotationOffset;

	if (bLeftHand)
	{
		HandMesh = Character->GetLeftHandMesh();
		MotionController = Character->LeftMotionController.Get();
		LocationOffset = Character->LeftHandMeshLocationOffset;
		RotationOffset = Character->LeftHandMeshRotationOffset;
	}
	else
	{
		HandMesh = Character->GetRightHandMesh();
		MotionController = Character->RightMotionController.Get();
		LocationOffset = Character->RightHandMeshLocationOffset;
		RotationOffset = Character->RightHandMeshRotationOffset;
	}

	if (HandMesh == nullptr || MotionController == nullptr) return;

	HandMesh->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HandMesh->SetRelativeLocationAndRotation(LocationOffset, RotationOffset);
}

void UCombatComponent::AttachMagazineToMotionController(AFullMagazine* MagToAttach, bool bLeftHand)
{
	if (Character == nullptr || MagToAttach == nullptr) return;
	UGripMotionControllerComponent* MotionController = bLeftHand ? Character->LeftMotionController.Get() : Character->RightMotionController.Get();
	if (MotionController == nullptr) return;

	FTransform OffsetTransform = bLeftHand ? MagToAttach->GrabOffsetLeft : MagToAttach->GrabOffsetRight;

	MagToAttach->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MagToAttach->SetActorRelativeTransform(OffsetTransform);
}

void UCombatComponent::HandleMagDrop(bool bLeftHand)
{
}

void UCombatComponent::OnRep_LeftEquippedWeapon()
{
	if (Character == nullptr) return;

	if (LeftEquippedWeapon)
	{
		LeftEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		//AttachActorToHand(LeftEquippedWeapon, true);
		HandleWeaponAttach(LeftEquippedWeapon, true);
		//PlayEquipWeaponSound(LeftEquippedWeapon);
		Character->SetHandState(true, EHandState::EHS_HoldingPistol);
		//LeftEquippedWeapon->SetHUDAmmo();
		return;
	}
	else
	{
		HandleWeaponDrop(true);
	}
	Character->SetHandState(true, EHandState::EHS_Idle);
}

void UCombatComponent::OnRep_RightEquippedWeapon()
{
	if (Character == nullptr) return;

	if (RightEquippedWeapon)
	{
		RightEquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		//AttachActorToHand(RightEquippedWeapon, false);
		HandleWeaponAttach(RightEquippedWeapon, false);
		// Handle
		//PlayEquipWeaponSound(RightEquippedWeapon);
		Character->SetHandState(false, EHandState::EHS_HoldingPistol);
		//RightEquippedWeapon->SetHUDAmmo();
		return;
	}
	else
	{
		HandleWeaponDrop(false);
	}
	Character->SetHandState(false, EHandState::EHS_Idle);
}

void UCombatComponent::Fire(bool bLeft)
{
	if (CanFire(bLeft))
	{
		const AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
		if (CurrentWeapon && Character)
		{
			bCanFire = false;

			switch (CurrentWeapon->FireType)
			{
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			}
			StartFireTimer(CurrentWeapon->FireDelay);
			Character->FireWeaponHaptic(bLeft);
		}
	}
	else if(bCanFire)
	{
		AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
		if (CurrentWeapon)
		{
			CurrentWeapon->PlayDryFireSound();
			StartFireTimer(CurrentWeapon->FireDelay);
		}
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (Character)
	{
		AWeapon* CharacterWeapon = Character->GetDefaultWeapon();
		if (CharacterWeapon)
		{
			HitTarget = CharacterWeapon->bUseScatter ? CharacterWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
			if (!Character->HasAuthority()) LocalFire(HitTarget);
			ServerFire(HitTarget, CharacterWeapon->FireDelay);
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (Character && Character->GetDefaultWeapon())
	{
		UWorld* World = GetWorld();
		const USkeletalMeshComponent* WeaponMesh = Character->GetDefaultWeapon()->GetWeaponMesh();
		if (!WeaponMesh || !World) return;
		const USkeletalMeshSocket* MuzzleSocket = Character->GetDefaultWeapon()->GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
		if (!MuzzleSocket) return;

		const FTransform SocketTransform = MuzzleSocket->GetSocketTransform(WeaponMesh);
		const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + MuzzleDirection * TRACE_LENGTH;

		World->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->GetDefaultWeapon() && CombatState == ECombatState::ECS_Unoccupied)
	{
		//Character->PlayFireMontage(bLeft);
		Character->GetDefaultWeapon()->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, const float FireDelay)
{
	MultiCastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, const float FireDelay)
{
	if (Character && Character->GetDefaultWeapon())
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(Character->GetDefaultWeapon()->FireDelay, FireDelay, 0.002f);
		return bNearlyEqual;
	}
	return false;
}

void UCombatComponent::MultiCastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(TraceHitTarget);
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
		Character->GetDefaultWeapon()->PlaySlideBackAnimation();
	}
}

bool UCombatComponent::CanFire(bool bLeft)
{
	const AWeapon* CurrentWeapon = GetEquippedWeapon(bLeft);
	if (CurrentWeapon == nullptr) return false;
	if (bLocallyReloading) return false;
	if (CurrentWeapon->IsObstructed()) return false;

	return !CurrentWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied && CurrentWeapon->IsMagInserted();
}

void UCombatComponent::DropWeapon(bool bLeftHand)
{
	if (Character == nullptr || Character->GetMesh() == nullptr) return;

	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HolsterSocket == nullptr) return;
	const FVector HolsterLocation = HolsterSocket->GetSocketLocation(Character->GetMesh());

	if (bLeftHand && LeftEquippedWeapon)
	{
		if (!HolsterWeaponDontDrop(bLeftHand, LeftEquippedWeapon, HolsterLocation))
		{
			LeftEquippedWeapon->Dropped(true);
			HandleWeaponDrop(true);
		}
		LeftEquippedWeapon = nullptr;
		OnRep_LeftEquippedWeapon();
	}
	else if (!bLeftHand && RightEquippedWeapon)
	{
		if (!HolsterWeaponDontDrop(bLeftHand, RightEquippedWeapon, HolsterLocation))
		{
			RightEquippedWeapon->Dropped(true);
			HandleWeaponDrop(false);
		}
		RightEquippedWeapon = nullptr;
		OnRep_RightEquippedWeapon();
	}
}

void UCombatComponent::ServerDropWeapon_Implementation(bool bLeft, FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity)
{
	if (bLeft && LeftEquippedWeapon)
	{
		LeftEquippedWeapon->MulticastDropWeapon(StartLocation, StartRotation, LinearVelocity, AngularVelocity);
		LeftEquippedWeapon = nullptr;
	}
	else if (!bLeft && RightEquippedWeapon)
	{
		RightEquippedWeapon->MulticastDropWeapon(StartLocation, StartRotation, LinearVelocity, AngularVelocity);
		RightEquippedWeapon = nullptr;
	}
}

void UCombatComponent::Reload()
{
	if (Character && Character->GetDefaultWeapon() && Character->GetDefaultWeapon()->GetCarriedAmmo() > 0 && CombatState == ECombatState::ECS_Unoccupied && !Character->GetDefaultWeapon()->IsFull() && !bLocallyReloading)
	{
		ServerReload();
		//HandleReload();
		bLocallyReloading = false;
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return;

	MulticastReload();
}

void UCombatComponent::MulticastReload_Implementation()
{
	HandleReload();
}

bool UCombatComponent::HolsterWeaponDontDrop(bool bLeftHand, AWeapon* WeaponToDrop, FVector HolsterLocation)
{
	if (WeaponToDrop)
	{
		FVector WeaponLocation = WeaponToDrop->GetActorLocation();
		float DistSquared = FVector::DistSquared(HolsterLocation, WeaponLocation);
		if (DistSquared < 1600) // 1.5ft
		{
			bLeftHand ? MulticastAttachToHolster(ESide::ES_Left) : MulticastAttachToHolster(ESide::ES_Right);
			return true;
		}
	}
	return false;
}

void UCombatComponent::HandleReload()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
	{
		UpdateWeaponAmmos();
	}
	CombatState = ECombatState::ECS_Unoccupied;
	Character->GetDefaultWeapon()->SetMagInserted(true);
	Character->GetDefaultWeapon()->PlaySlideForwardAnimation();

	if (!Character->IsEliminated())
	{
		Character->GetDefaultWeapon()->PlayReloadSound();
	}
}

void UCombatComponent::AttachWeaponToHolster(AWeapon* WeaponToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || WeaponToAttach == nullptr || WeaponToAttach->GetWeaponMesh() == nullptr) return;
	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HolsterSocket)
	{
		HolsterSocket->AttachActor(WeaponToAttach, Character->GetMesh());
		WeaponToAttach->SetActorRelativeLocation(FVector::ZeroVector);
		WeaponToAttach->SetActorRelativeRotation(FRotator::ZeroRotator);
		WeaponToAttach->GetWeaponMesh()->SetSimulatePhysics(false);
		WeaponToAttach->GetWeaponMesh()->SetEnableGravity(false);
		WeaponToAttach->GetWeaponMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
}

void UCombatComponent::MulticastAttachToHolster_Implementation(ESide HandSide)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	const FName SocketName = Character->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
	const USkeletalMeshSocket* HolsterSocket = Character->GetMesh()->GetSocketByName(SocketName);
	AWeapon* Weapon = Character->GetDefaultWeapon();

	if (Weapon->GetWeaponMesh() == nullptr || Weapon->GetAreaSphere() == nullptr) return;

	Weapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	Weapon->GetWeaponMesh()->SetEnableGravity(false);
	Weapon->GetWeaponMesh()->SetSimulatePhysics(false);
	Weapon->GetWeaponMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Weapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	Weapon->SetActorRelativeLocation(FVector::ZeroVector);
	Weapon->SetActorRelativeRotation(FRotator::ZeroRotator);

	if (Character->IsLocallyControlled() && !Character->IsEliminated())
	{
		Weapon->PlayHolsterSound();
	}

	Weapon->SetWeaponState(EWeaponState::EWS_Holstered);

	if (HandSide == ESide::ES_None) return;

	HandSide == ESide::ES_Left ? HandleWeaponDrop(true) : HandleWeaponDrop(false);
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{

}

void UCombatComponent::OnRep_CombatState()
{
	/*
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OnRep Reloading"));
		}
		break;
	case ECombatState::ECS_Unoccupied:
		if (Character && Character->GetDefaultWeapon())
		{
			Character->GetDefaultWeapon()->PlaySlideForwardAnimation();
			//
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OnRep Unocuupied"));
			}
		}
		break;
	}*/

}

void UCombatComponent::UpdateWeaponAmmos()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return;
	AWeapon* DefaultWeapon = Character->GetDefaultWeapon();

	int32 ReloadAmount = AmountToReload();

	DefaultWeapon->SetCarriedAmmo(DefaultWeapon->GetCarriedAmmo() - ReloadAmount);

	Controller = Controller == nullptr ? Cast<AGunfightPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		//Controller->SetHUDCarriedAmmo(EquippedWeapon->GetCarriedAmmo());
	}
	DefaultWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateCarriedAmmo()
{
}

void UCombatComponent::UpdateAmmoValues()
{
}

int32 UCombatComponent::AmountToReload()
{
	if (Character == nullptr || Character->GetDefaultWeapon() == nullptr) return 0;
	const AWeapon* Weapon = Character->GetDefaultWeapon();
	int32 RoomInMag = Weapon->GetMagCapacity() - Weapon->GetAmmo();
	int32 AmountCarried = Weapon->GetCarriedAmmo();
	int32 Least = FMath::Min(RoomInMag, AmountCarried);
	return FMath::Clamp(RoomInMag, 0, Least);
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
