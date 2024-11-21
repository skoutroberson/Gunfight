// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "GunfightCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightCharacter : public AVRCharacter
{
	GENERATED_BODY()
	
public:
	
	AGunfightCharacter();
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void SetVREnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	// input
	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	void TurnRight(float Throttle);
	void LookUp(float Throttle);

private:
	class UGunfightAnimInstance* GunfightAnimInstance;

	void UpdateAnimInstanceIK();
	FVector TraceFootLocationIK(bool bLeft);

	void MoveMeshToCamera();

	void UpdateAnimation();

	UPROPERTY()
	UWorld* World;

	FCollisionQueryParams IKQueryParams;

	UPROPERTY(VisibleAnywhere);
	bool bIsInVR = true;

public:
	
};
