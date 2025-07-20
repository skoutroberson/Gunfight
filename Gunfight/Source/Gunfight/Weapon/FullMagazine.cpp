// Fill out your copyright notice in the Description page of Project Settings.


#include "FullMagazine.h"
#include "Gunfight/Gunfight.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"

AFullMagazine::AFullMagazine()
{
	PrimaryActorTick.bCanEverTick = true;

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	SetRootComponent(MagazineMesh);
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionObjectType(ECC_Weapon);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetUseCCD(true);
	AreaSphere->SetSphereRadius(0.f);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetUseCCD(true);
}

void AFullMagazine::SetWeaponSkin(int32 SkinIndex)
{
	if (SkinIndex >= WeaponSkins.Num() || MagazineMesh == nullptr) return;

	MagazineMesh->SetMaterial(0, WeaponSkins[SkinIndex]);
}

void AFullMagazine::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AFullMagazine::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AFullMagazine::OnSphereEndOverlap);

	MagazineMesh->SetMaskFilterOnBodyInstance(1 << 3); // so player leg IK ignores this

	GetWorldTimerManager().SetTimer(InitializeCollisionHandle, this, &AFullMagazine::InitializeCollision, 0.02f, false, 0.1f);
}

void AFullMagazine::GetSpawnOverlaps()
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
				GunfightCharacter->SetOverlappingMagazine(this);
				bLeftControllerOverlap = true;
			}
			if (RightDistance <= FMath::Square(RightHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bRightControllerOverlap = true;
				GunfightCharacter->SetOverlappingMagazine(this);
			}
		}
	}
}

void AFullMagazine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bLerpToMagwellStart)
	{
		LerpToMagwellStart(DeltaTime);
	}
}

void AFullMagazine::Dropped()
{
	bEquipped = false;
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	MagazineMesh->DetachFromComponent(DetachRules);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MagazineMesh->SetSimulatePhysics(true);
	MagazineMesh->SetEnableGravity(true);
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MagazineMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	MagazineMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	MagazineMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetWorldTimerManager().SetTimer(ShouldAttachToHolsterHandle, this, &AFullMagazine::ShouldAttachToHolster, 0.5f, false, 0.5f);
	GetWorldTimerManager().ClearTimer(InsertIntoMagHandle);
}

void AFullMagazine::Equipped()
{
	bEquipped = true;
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MagazineMesh->SetSimulatePhysics(false);
	MagazineMesh->SetEnableGravity(false);
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetWorldTimerManager().SetTimer(InsertIntoMagHandle, this, &AFullMagazine::CanInsertIntoMagwell, 0.1f, true, 0.1f);
}

void AFullMagazine::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == CharacterOwner)
	{
		if (OtherComp == CharacterOwner->GetLeftHandSphere())
		{
			bLeftControllerOverlap = true;
		}
		else if (OtherComp == CharacterOwner->GetRightHandSphere()) bRightControllerOverlap = true;
		CharacterOwner->SetOverlappingMagazine(this);
	}
}

void AFullMagazine::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (CharacterOwner && OtherActor == CharacterOwner)
	{
		if (OtherComp == CharacterOwner->GetLeftHandSphere())
		{
			bLeftControllerOverlap = false;
			if (!bRightControllerOverlap) CharacterOwner->SetOverlappingMagazine(nullptr);
		}
		else if (OtherComp == CharacterOwner->GetRightHandSphere())
		{
			bRightControllerOverlap = false;
			if (!bLeftControllerOverlap) CharacterOwner->SetOverlappingMagazine(nullptr);
		}
		//GunfightCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AFullMagazine::InitializeCollision()
{
	AreaSphere->SetSphereRadius(7.f, true);
	GetSpawnOverlaps();
}

void AFullMagazine::ShouldAttachToHolster()
{
	if (CharacterOwner)
	{
		UCombatComponent* CombatComponent = CharacterOwner->GetCombat();
		if (!CombatComponent->GetEquippedMagazine(false) && !CombatComponent->GetEquippedMagazine(false))
		{
			if (FVector::DistSquared(CharacterOwner->GetActorLocation(), GetActorLocation()) > 14400.f)
			{
				MagazineMesh->SetSimulatePhysics(false);
				MagazineMesh->SetEnableGravity(false);
				CharacterOwner->AttachMagazineToHolster(this, GetMagHolsterSide());
				return;
			}
		}
		else
		{
			return;
		}
		GetWorldTimerManager().SetTimer(ShouldAttachToHolsterHandle, this, &AFullMagazine::ShouldAttachToHolster, 0.5f, false, 0.5f);
	}
}

void AFullMagazine::CanInsertIntoMagwell()
{
	if (CharacterOwner)
	{
		UCombatComponent* Combat = CharacterOwner->GetCombat();
		if (WeaponOwner && Combat)
		{
			const FVector MagwellDirection = WeaponOwner->GetMagwellDirection();
			const FVector UV = GetActorUpVector();
			const float Dot = FVector::DotProduct(MagwellDirection, UV);
			const float DistanceSquared = FVector::DistSquared(WeaponOwner->GetMagwellEnd()->GetComponentLocation(), GetActorLocation());

			if (DistanceSquared < 75.f && Dot > 0.85f)
			{
				GetWorldTimerManager().ClearTimer(InsertIntoMagHandle);

				if (Combat->GetEquippedMagazine(true) == this)
				{
					Combat->SetEquippedMagazine(nullptr, true);
				}
				else if (Combat->GetEquippedMagazine(false) == this)
				{
					Combat->SetEquippedMagazine(nullptr, false);
				}

				DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				AttachToComponent(WeaponOwner->GetMagwellEnd(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				MagwellEnd = WeaponOwner->GetMagwellEnd();
				MagwellStart = WeaponOwner->GetMagwellStart();
				MagwellRelativeTargetLocation = MagwellStart->GetRelativeLocation() - MagwellEnd->GetRelativeLocation();
				bLerpToMagwellStart = true;
				SetActorTickEnabled(true);

				// TODO: Play mag insert sound
			}
		}
	}
}

void AFullMagazine::LerpToMagwellStart(float DeltaTime)
{
	if (CharacterOwner && CharacterOwner->GetCombat() && WeaponOwner)
	{
		const float MagSpeedScaled = MagInsertSpeed * DeltaTime;
		MagwellDistance = MagwellDistance + MagSpeedScaled > 1 ? 1.0f : MagwellDistance + MagSpeedScaled;
		SetActorRelativeLocation(MagwellRelativeTargetLocation * MagwellDistance);
		if (MagwellDistance == 1.0f)
		{
			bLerpToMagwellStart = false;
			SetActorTickEnabled(false);
			WeaponOwner->UnhideMag();

			if (CharacterOwner->GetCombat()->GetEquippedWeapon(true) == WeaponOwner)
			{
				CharacterOwner->GetCombat()->Reload(true);
			}
			else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false) == WeaponOwner)
			{
				CharacterOwner->GetCombat()->Reload(false);
			}
			//CharacterOwner->GetCombat()->Reload(bLeft);
			WeaponOwner->SetMagazine(nullptr);
			Destroy();
		}
	}
}

ESide AFullMagazine::GetMagHolsterSide()
{
	if (WeaponOwner == nullptr || CharacterOwner == nullptr || CharacterOwner->GetCombat() == nullptr) return ESide::ES_None;

	if (CharacterOwner->GetCombat()->GetEquippedWeapon(true) == WeaponOwner)
	{
		return ESide::ES_Right; // mag holster is opposite of the weapon
	}
	else if (CharacterOwner->GetCombat()->GetEquippedWeapon(false) == WeaponOwner)
	{
		return ESide::ES_Left; // mag holster is opposite of the weapon
	}

	return ESide::ES_None;
}
