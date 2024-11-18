// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GunfightCharacter.generated.h"

UENUM(BlueprintType)
enum class EHandState : uint8
{
	EES_Idle			UMETA(DisplayName = "Idle"),
	EES_Grip			UMETA(DisplayName = "Grip"),
	EES_HoldingPistol	UMETA(DisplayName = "HoldingPistol"),

	EHS_MAX				UMETA(DisplayName = "DefaultMAX"),
};

UCLASS()
class GUNFIGHT_API AGunfightCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGunfightCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	void TurnRight(float Throttle);

	DECLARE_DELEGATE_OneParam(FLeftRightButtonDelegate, const bool);

	void GripPressed(bool bLeftController);
	void GripReleased(bool bLeftController);
	void TriggerPressed(bool bLeftController);
	void TriggerReleased(bool bLeftController);
	void AButtonPressed(bool bLeftController);
	void AButtonReleased(bool bLeftController);
	void BButtonPressed(bool bLeftController);
	void BButtonReleased(bool bLeftController);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	bool UsingHMD = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandController, meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* RightHandController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandController, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* LeftHandController;

	UPROPERTY(VisibleAnywhere, Category = HandController)
	class USphereComponent* RightHandSphere;

	UPROPERTY(VisibleAnywhere, Category = HandController)
	USphereComponent* LeftHandSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bRightHolsterPreferred = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gun, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UChildActorComponent> DefaultWeapon;

	UPROPERTY()
	class AWeapon* ChildWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	FVector CameraMeshOffset = FVector(-12.373209f, 0.f, -177.667695f);
	//(X = -12.373209, Y = 0.000000, Z = -177.667695)

	UPROPERTY(EditAnywhere)
	EHandState LeftHandState;

	UPROPERTY(EditAnywhere)
	EHandState RightHandState;

	/**
	* Gunfight components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRComponent> VRComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class ULagCompensationComponent> LagCompensation;

	UPROPERTY()
	class AWeapon* OverlappingWeapon;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(bool bLeftController);
	void ServerEquipButtonPressed_Implementation(bool bLeftController);

	void PollInit();

public:	

	/**
	* VRComponent RPCs
	*/


	UFUNCTION(Server, Unreliable)
	void ServerVRMove(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	void ServerVRMove_Implementation(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	bool ServerVRMove_Validate(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	UFUNCTION(Server, Unreliable)
	void MulticastVRMove(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	void MulticastVRMove_Implementation(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	bool MulticastVRMove_Validate(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

private:
	inline const bool ValidateVRMovement(float Delay);

public:
	inline UCameraComponent* GetCamera() { return Camera; }
	inline UMotionControllerComponent* GetHandController(bool bLeft) { return bLeft ? LeftHandController : RightHandController; }
	inline UMotionControllerComponent* GetLeftHandController() { return LeftHandController; }
	inline UMotionControllerComponent* GetRightHandController() { return RightHandController; }
	AWeapon* GetWeapon();
	inline void SetOverlappingWeapon(AWeapon* Weapon) { OverlappingWeapon = Weapon; }
};
