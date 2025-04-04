// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine\PhysicsAsset.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Gunfight.h"
#include "Gunfight/Character/GunfightCharacterDeprecated.h"
#include "MotionControllerComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/Weapon/EmptyMagazine.h"
#include "Kismet/KismetMathLibrary.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "GripMotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // this needs to be set true so a late joining player has OnRep_CurrentSkinIndex called for weapons already in the game.

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionObjectType(ECC_Weapon);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetUseCCD(true);
	AreaSphere->SetSphereRadius(10.f);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	MagSlideEnd = CreateDefaultSubobject<USceneComponent>(TEXT("MagSlideEnd"));
	MagSlideEnd->SetupAttachment(WeaponMesh);
	MagSlideStart = CreateDefaultSubobject<USceneComponent>(TEXT("MagSlideStart"));
	MagSlideStart->SetupAttachment(WeaponMesh);
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME_CONDITION(AWeapon, CarriedMags, COND_OwnerOnly);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
	DOREPLIFETIME(AWeapon, WeaponSide);
	DOREPLIFETIME(AWeapon, CurrentSkinIndex);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	SpendRound();
	if (FireAnimation && FireEndAnimation && CharacterOwner)
	{
		if (Ammo <= 0 && CharacterOwner->IsLocallyControlled())
		{
			WeaponMesh->PlayAnimation(FireEndAnimation, false);
		}
		else
		{
			WeaponMesh->PlayAnimation(FireAnimation, false);
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	if (CharacterOwner)
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController)
		{
			GunfightOwnerController->SetHUDWeaponAmmo(Ammo);
			GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
}

void AWeapon::UnhideMag()
{
	if (WeaponMesh)
	{
		WeaponMesh->UnHideBoneByName(FName("Colt_Magazine"));
	}
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::PlayReloadSound()
{
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
}

void AWeapon::PlayHolsterSound()
{
	if (HolsterSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HolsterSound, GetActorLocation());
	}
}

void AWeapon::PlayDryFireSound()
{
	if (DryFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetActorLocation());
	}
}

void AWeapon::SetWeaponSkin(int32 SkinIndex)
{
	if (SkinIndex >= WeaponSkins.Num() || SkinIndex < 0) return;

	CurrentSkinIndex = SkinIndex;
	WeaponMesh->SetMaterial(0, WeaponSkins[SkinIndex]);
}

void AWeapon::ServerSetWeaponSkin_Implementation(int32 SkinIndex)
{
	SetWeaponSkin(SkinIndex);
}

bool AWeapon::IsObstructed() const
{
	if (!GetWorld()) return false;
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return false;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
	const FVector MuzzleDirection = SocketTransform.GetRotation().GetForwardVector();
	const FVector Start = SocketTransform.GetLocation() - MuzzleDirection; // front of the barrel
	const FVector End = Start - MuzzleDirection * ObstructedDistance;	// back of the gun

	FHitResult HitResult;
	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_WorldDynamic);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	WeaponMesh->SetMaskFilterOnBodyInstance(1 << 3); // Player leg IK will ignore the weapon mesh

	BulletQueryParams.bReturnPhysicalMaterial = true;

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);

	// initialize on spawn sphere overlaps
	SetActorTickEnabled(true);

	/*
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ShouldHolsterTimer, this, &AWeapon::ShouldAttachToHolster, 1.f, true, 1.f);
	}
	*/
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && (GetOwner() == GunfightCharacter || GetOwner() == nullptr))
	{
		if (OtherComp == GunfightCharacter->GetLeftHandSphere()) bLeftControllerOverlap = true;
		else if (OtherComp == GunfightCharacter->GetRightHandSphere()) bRightControllerOverlap = true;
		GunfightCharacter->SetOverlappingWeapon(this);

		GunfightCharacter->OverlappingWeapons.AddUnique(this);

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, FString::Printf(TEXT("Overlap: %d"), GunfightCharacter->GetOverlappingWeapon()));
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(OtherActor);
	if (GunfightCharacter && (GetOwner() == GunfightCharacter || GetOwner() == nullptr))
	{
		if (OtherComp == GunfightCharacter->GetLeftHandSphere())
		{
			bLeftControllerOverlap = false;
			if(!bRightControllerOverlap) GunfightCharacter->OverlappingWeapons.RemoveSingle(this);
		}
		else if (OtherComp == GunfightCharacter->GetRightHandSphere())
		{
			bRightControllerOverlap = false;
			if(!bLeftControllerOverlap) GunfightCharacter->OverlappingWeapons.RemoveSingle(this);
		}

		GunfightCharacter->OverlappingWeapons.IsEmpty() ? GunfightCharacter->SetOverlappingWeapon(nullptr) : GunfightCharacter->SetOverlappingWeapon(GunfightCharacter->OverlappingWeapons[0]);

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta, FString::Printf(TEXT("End Overlap: %d"), CharacterOwner->GetOverlappingWeapon()));
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	PollInit();
}

void AWeapon::PollInit()
{
	if (!bInitDelayCompleted)
	{
		if (InitDelayFrame < 1)
		{
			InitDelayFrame++;
		}
		else
		{
			bInitDelayCompleted = true;
			SetActorTickEnabled(false);
			GetSpawnOverlaps();
		}
	}
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	SetHUDAmmo();
}

void AWeapon::OnRep_CarriedAmmo()
{
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner && CharacterOwner->GetDefaultWeapon())
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController && CharacterOwner->GetDefaultWeapon() == this)
		{
			GunfightOwnerController->SetHUDCarriedAmmo(CarriedAmmo);
		}
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else if (CharacterOwner && CharacterOwner->IsLocallyControlled())
	{
		++Sequence;
	}
}

void AWeapon::EjectMagazine()
{
	if (bMagInserted && MagEjectAnimation)
	{
		bMagInserted = false;
		WeaponMesh->PlayAnimation(MagEjectAnimation, false);
		GetWorldTimerManager().SetTimer(EjectMagTimerHandle, this, &AWeapon::DropMag, MagDropDelay, false, MagDropDelay);
	}
}

bool AWeapon::CheckHandOverlap(bool bLeftHand)
{
	return bLeftHand ? bLeftControllerOverlap : bRightControllerOverlap;
}

FVector AWeapon::GetMagwellDirection() const
{
	return (MagSlideStart->GetComponentLocation() - MagSlideEnd->GetComponentLocation()).GetSafeNormal();
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMesh->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMesh);
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AWeapon::DropMag()
{
	if (WeaponMesh == nullptr) return;

	WeaponMesh->HideBoneByName(FName("Colt_Magazine"), EPhysBodyOp::PBO_None);
	WeaponMesh->PlayAnimation(SlideBackAnimationPose, false);

	if (EmptyMagazineClass)
	{
		const USkeletalMeshSocket* MeshSocket = WeaponMesh->GetSocketByName(TEXT("EmptyMag"));
		if (MeshSocket)
		{
			const FTransform SocketTransform = MeshSocket->GetSocketTransform(WeaponMesh);
			const FVector ImpulseDir = MagSlideEnd->GetComponentLocation() - MagSlideStart->GetComponentLocation();

			UWorld* World = GetWorld();
			if (World)
			{
				AEmptyMagazine* EmptyMag = World->SpawnActor<AEmptyMagazine>(EmptyMagazineClass, SocketTransform);
				if (EmptyMag && CharacterOwner && CharacterOwner->GetCombat())
				{
					if (EmptyMag->GetMagazineMesh() && WeaponMesh)
					{
						EmptyMag->GetMagazineMesh()->SetMaterial(0, WeaponMesh->GetMaterial(0));
					}

					if (CharacterOwner->GetCombat()->GetEquippedWeapon(true))
					{
						EmptyMag->GetMagazineMesh()->SetPhysicsLinearVelocity(CharacterOwner->GetLeftMotionControllerAverageVelocity() * 60.f);
						EmptyMag->GetMagazineMesh()->SetPhysicsAngularVelocityInRadians(CharacterOwner->LeftMotionControllerAverageAngularVelocity * 5.f);
					}
					else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false))
					{
						EmptyMag->GetMagazineMesh()->SetPhysicsLinearVelocity(CharacterOwner->GetRightMotionControllerAverageVelocity() * 60.f);
						EmptyMag->GetMagazineMesh()->SetPhysicsAngularVelocityInRadians(CharacterOwner->RightMotionControllerAverageAngularVelocity * 5.f);
					}
					EmptyMag->GetMagazineMesh()->AddImpulse(FVector(ImpulseDir * MagDropImpulse));
				}
			}
		}
	}
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	WeaponState = NewState;
	OnWeaponStateSet();
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}

void AWeapon::OnEquipped()
{
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	//WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);

	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner)
	{
		GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
		if (GunfightOwnerController && HasAuthority() && !GunfightOwnerController->HighPingDelegate.IsBound())
		{
			GunfightOwnerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
		}

		if (CharacterOwner->IsLocallyControlled() && bPlayingEquipSound == false)
		{
			if (EquipSound)
			{
				bPlayingEquipSound = true;
				UGameplayStatics::PlaySoundAtLocation(this, EquipSound, GetActorLocation());
				GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AWeapon::EquipSoundTimerEnd, 0.5f, false, 0.5f);
			}
		}
	}
}

void AWeapon::MulticastDropWeapon_Implementation(FVector_NetQuantize StartLocation, FRotator StartRotation, FVector_NetQuantize LinearVelocity, FVector_NetQuantize AngularVelocity)
{
	if (WeaponMesh == nullptr || AreaSphere == nullptr) return;

	WeaponState = EWeaponState::EWS_Dropped;
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);

	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner == nullptr || CharacterOwner->GetCombat() == nullptr) return;



	GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
	if (GunfightOwnerController && HasAuthority() && GunfightOwnerController->HighPingDelegate.IsBound())
	{
		GunfightOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}

	SetActorLocationAndRotation(StartLocation, StartRotation);
	WeaponMesh->SetPhysicsLinearVelocity(LinearVelocity);
	WeaponMesh->SetPhysicsAngularVelocityInRadians(AngularVelocity);
}

void AWeapon::Dropped(bool bLeftHand)
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	CharacterOwner = nullptr;
}

void AWeapon::OnDropped()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	
	CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : CharacterOwner;
	if (CharacterOwner == nullptr) return;

	GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(CharacterOwner->Controller) : GunfightOwnerController;
	if (GunfightOwnerController && HasAuthority() && GunfightOwnerController->HighPingDelegate.IsBound())
	{
		GunfightOwnerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
	}

	// apply hand controller velocities on drop
	if (WeaponSide == ESide::ES_Left)
	{
		WeaponMesh->SetPhysicsLinearVelocity(CharacterOwner->GetLeftMotionControllerAverageVelocity() * 50.f);
		WeaponMesh->SetPhysicsAngularVelocityInRadians(CharacterOwner->LeftMotionControllerAverageAngularVelocity * 14.f);
	}
	else if (WeaponSide == ESide::ES_Right)
	{
		WeaponMesh->SetPhysicsLinearVelocity(CharacterOwner->GetRightMotionControllerAverageVelocity() * 50.f);
		WeaponMesh->SetPhysicsAngularVelocityInRadians(CharacterOwner->RightMotionControllerAverageAngularVelocity * 14.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Dropped weapon WeaponSide is not set"));
	}

	// set owner to nullptr
	WeaponSide = ESide::ES_None;
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::PlaySlideBackAnimation()
{
	if (SlideBackAnimation)
	{
		WeaponMesh->PlayAnimation(SlideBackAnimation, false);
	}
}

void AWeapon::PlaySlideForwardAnimation()
{
	if (SlideForwardAnimation)
	{
		WeaponMesh->PlayAnimation(SlideForwardAnimation, false);
	}
}

void AWeapon::SetServerSideRewind(bool bUseSSR)
{
	bUseServerSideRewind = bUseSSR;
}

void AWeapon::ShouldAttachToHolster()
{
	if (HasAuthority() && CharacterOwner && CharacterOwner->GetMesh())
	{
		UCombatComponent* Combat = CharacterOwner->GetCombat();
		if (Combat && !Combat->GetEquippedWeapon(true) && !Combat->GetEquippedWeapon(false))
		{
			const FName SocketName = CharacterOwner->GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
			const USkeletalMeshSocket* HolsterSocket = CharacterOwner->GetMesh()->GetSocketByName(SocketName);
			if (HolsterSocket == nullptr) return;

			const float DistSquared = FVector::DistSquared(HolsterSocket->GetSocketLocation(CharacterOwner->GetMesh()), GetActorLocation());
			if (DistSquared > 45522.f) // 7ft
			{
				Combat->MulticastAttachToHolster(ES_None);
			}
		}
	}
}

void AWeapon::GetSpawnOverlaps()
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetOwner());
	if (GunfightCharacter)
	{
		USphereComponent* LeftHandSphere = GunfightCharacter->GetLeftHandSphere();
		USphereComponent* RightHandSphere = GunfightCharacter->GetRightHandSphere();

		if (LeftHandSphere && RightHandSphere)
		{
			const float LeftDistance = FVector::DistSquared(LeftHandSphere->GetComponentLocation(), GetActorLocation());
			const float RightDistance = FVector::DistSquared(RightHandSphere->GetComponentLocation(), GetActorLocation());

			if (LeftDistance <= FMath::Square(LeftHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bLeftControllerOverlap = true;
			}
			if (RightDistance <= FMath::Square(RightHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bRightControllerOverlap = true;
			}
		}
	}
}

void AWeapon::OnRep_CurrentSkinIndex()
{
	SetWeaponSkin(CurrentSkinIndex);
}

bool AWeapon::IsOverlappingHand(bool bLeftHand)
{
	AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(GetOwner());
	if (GunfightCharacter == nullptr) return false;
	USphereComponent* HandSphere = bLeftHand ? GunfightCharacter->GetLeftHandSphere() : GunfightCharacter->GetRightHandSphere();
	if (HandSphere == nullptr) return false;
	const float Distance = FVector::DistSquared(HandSphere->GetComponentLocation(), AreaSphere->GetComponentLocation());

	if (Distance <= FMath::Square(HandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
	{
		return true;
	}
	return false;
}

void AWeapon::ApplyVelocityOnDropped()
{
	WeaponMesh->SetPhysicsAngularVelocityInRadians(DropAngularVelocity.Vector());
	WeaponMesh->SetPhysicsLinearVelocity(DropVelocity);
}

void AWeapon::EquipSoundTimerEnd()
{
	bPlayingEquipSound = false;
}
