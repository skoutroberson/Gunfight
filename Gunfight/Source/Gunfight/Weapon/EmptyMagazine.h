// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmptyMagazine.generated.h"

UCLASS()
class GUNFIGHT_API AEmptyMagazine : public AActor
{
	GENERATED_BODY()
	
public:	
	AEmptyMagazine();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MagazineMesh;

	UPROPERTY(EditAnywhere)
	float MagEjectionImpulse;

	UPROPERTY(EditAnywhere)
	class USoundCue* MagSound;

	FTimerHandle DestroyTimerHandle;

	void DestroyMagazine();

public:	
	FORCEINLINE UPrimitiveComponent* GetMagazineMesh() { return MagazineMesh; }
	
};
