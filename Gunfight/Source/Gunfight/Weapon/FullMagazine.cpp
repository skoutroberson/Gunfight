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

void AFullMagazine::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AFullMagazine::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AFullMagazine::OnSphereEndOverlap);

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
				bLeftControllerOverlap = true;
			}
			if (RightDistance <= FMath::Square(RightHandSphere->GetScaledSphereRadius() + AreaSphere->GetScaledSphereRadius()))
			{
				bRightControllerOverlap = true;
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("FullMagOverlap"));
		}
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
	AreaSphere->SetSphereRadius(100.f, true);
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
				CharacterOwner->AttachMagazineToHolster();
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
		AWeapon* Weapon = CharacterOwner->GetDefaultWeapon();
		if (Weapon && Combat)
		{
			const FVector MagwellDirection = Weapon->GetMagwellDirection();
			const FVector UV = GetActorUpVector();
			const float Dot = FVector::DotProduct(MagwellDirection, UV);
			const float DistanceSquared = FVector::DistSquared(Weapon->GetMagwellEnd()->GetComponentLocation(), GetActorLocation());

			if (DistanceSquared < 7000.f && Dot > -0.9f)
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
				AttachToComponent(Weapon->GetMagwellEnd(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				MagwellEnd = Weapon->GetMagwellEnd();
				MagwellStart = Weapon->GetMagwellStart();
				MagwellRelativeTargetLocation = MagwellStart->GetRelativeLocation() - MagwellEnd->GetRelativeLocation();
				bLerpToMagwellStart = true;
				SetActorTickEnabled(true);
			}
		}
	}
}

void AFullMagazine::LerpToMagwellStart(float DeltaTime)
{
	if (CharacterOwner && CharacterOwner->GetCombat())
	{
		AWeapon* Weapon = CharacterOwner->GetDefaultWeapon();
		if (Weapon)
		{
			const float MagSpeedScaled = MagInsertSpeed * DeltaTime;
			MagwellDistance = MagwellDistance + MagSpeedScaled > 1 ? 1.0f : MagwellDistance + MagSpeedScaled;
			SetActorRelativeLocation(MagwellRelativeTargetLocation * MagwellDistance);
			if (MagwellDistance == 1.0f)
			{
				bLerpToMagwellStart = false;
				SetActorTickEnabled(false);
				Weapon->UnhideMag();
				CharacterOwner->GetCombat()->Reload();
				Destroy();
			}
		}
	}
}
