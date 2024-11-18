// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRComponent.generated.h"

//=============================================================================
/**
 * VRComponent handles VR logic for the associated GunfightCharacter owner
 * 
 * Includes replicated movement for the hand controllers and camera (HMD)
 */


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API UVRComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVRComponent();
	friend class AGunfightCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

/**
 * The actual network RPCs for VR component replication are passed to AGunfightCharacter, which wrap to the _ClientReceive function here, to avoid Component RPC overhead.
 * For example:
 *			Client: UVRComponent::ServerVRMove_ClientSend() => calls CharacterOwner->ServerVRMove() triggering RPC on the server.
 *			Server: AGunfightCharacter::ServerVRMove_Implementation() from the RPC => Calls CharacterOwner->MulticastVRMove() triggering Multicast RPC on the server.
 *			Server: AGunfightCharacter::MulticastVRMove_Implementation() from the RPC => Calls UVRComponent::VRMove_ClientReceive() on the server and clients.
 * 
 * UVRComponent::ServerVRMove_ClientSend() => CharacterOwner->ServerVRMove() => CharacterOwner->MulticastVRMove() => UVRComponent::VRMove_ClientReceive()
 *
 *  VRComponent calls ServerVRMove_ClientSend() every ReplicateVRDelay seconds.
*/

	void ServerVRMove_ClientSend(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	void VRMove_ClientReceive(
		const FVector_NetQuantize CameraLocation,
		const FRotator CameraRotation,
		const FVector_NetQuantize LeftHandLocation,
		const FRotator LeftHandRotation,
		const FVector_NetQuantize RightHandLocation,
		const FRotator RightHandRotation,
		float Delay
	);

	UPROPERTY(EditAnywhere, Category = VRComponent)
	float ReplicateVRDelay = 0.04f;


protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<class AGunfightCharacter> CharacterOwner;

private:

	/**
	* VR component replication
	*/

	// updates the server with HMD and motion controller location and rotation
	void ReplicateVRMovement(float DeltaTime);
	// interpolates HMD and motion controller location and rotation for owning character if this is a simulated proxy (or on the server and not locally controlled)
	void InterpVRMovement(float DeltaTime);

	float CurrentDelay = 0.f;

	UPROPERTY(VisibleAnywhere)
	FVector_NetQuantize GoalCameraLocation;
	UPROPERTY(VisibleAnywhere)
	FRotator GoalCameraRotation;
	UPROPERTY(VisibleAnywhere)
	FVector_NetQuantize GoalLeftHandLocation;
	UPROPERTY(VisibleAnywhere)
	FRotator GoalLeftHandRotation;
	UPROPERTY(VisibleAnywhere)
	FVector_NetQuantize GoalRightHandLocation;
	UPROPERTY(VisibleAnywhere)
	FRotator GoalRightHandRotation;

public:	
	
};
