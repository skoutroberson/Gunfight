// Fill out your copyright notice in the Description page of Project Settings.


#include "EmptyMagazine.h"


AEmptyMagazine::AEmptyMagazine()
{
	PrimaryActorTick.bCanEverTick = false;

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	SetRootComponent(MagazineMesh);
	MagazineMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	MagazineMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	MagazineMesh->SetSimulatePhysics(true);
	MagazineMesh->SetEnableGravity(true);
	MagazineMesh->SetNotifyRigidBodyCollision(true);
}

void AEmptyMagazine::BeginPlay()
{
	Super::BeginPlay();
	
	MagazineMesh->OnComponentHit.AddDynamic(this, &AEmptyMagazine::OnHit);
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AEmptyMagazine::DestroyMagazine, 3.f, false, 3.f);
}

void AEmptyMagazine::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	MagazineMesh->SetNotifyRigidBodyCollision(false);
}

void AEmptyMagazine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEmptyMagazine::DestroyMagazine()
{
	Destroy();
}