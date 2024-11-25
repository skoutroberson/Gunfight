// Fill out your copyright notice in the Description page of Project Settings.


#include "FullMagazine.h"
#include "Gunfight/Gunfight.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"

AFullMagazine::AFullMagazine()
{
	PrimaryActorTick.bCanEverTick = false;

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
}

void AFullMagazine::BeginPlay()
{
	Super::BeginPlay();

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECC_HandController, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AFullMagazine::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AFullMagazine::OnSphereEndOverlap);
	
	GetWorldTimerManager().SetTimer(InitializeCollisionHandle, this, &AFullMagazine::InitializeCollision, 0.011f, false, 0.011f);
}

void AFullMagazine::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == CharacterOwner)
	{
		CharacterOwner->SetOverlappingMagazine(this);
	}
}

void AFullMagazine::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == CharacterOwner)
	{
		TArray<AActor*> OverlappingActors;
		AreaSphere->GetOverlappingActors(OverlappingActors);

		if (OverlappingActors.IsEmpty())
		{
			CharacterOwner->SetOverlappingMagazine(nullptr);
		}
	}
}

void AFullMagazine::InitializeCollision()
{
	AreaSphere->SetSphereRadius(100.f, true);
}