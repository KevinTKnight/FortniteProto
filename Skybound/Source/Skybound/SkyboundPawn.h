// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SkyboundPawn.generated.h"

class UPhysicalMaterial;
class UCameraComponent;
class USpringArmComponent;
class UTextRenderComponent;
class UInputComponent;
class UAudioComponent;

UCLASS(config=Game)
class ASkyboundPawn : public APawn
{
	GENERATED_BODY()

	/** Spring arm that will offset the camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArm;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** SCene component for the In-Car view origin */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* InternalCameraBase;

	/** Camera component for the In-Car view */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* InternalCamera;

	/** Text component for the In-Car speed */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent* InCarSpeed;

	/** Text component for the In-Car gear */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent* InCarGear;

	/** Audio component for the engine sound */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* EngineSoundComponent;

public:
	ASkyboundPawn();

	/** The current speed as a string eg 10 km/h */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
	FText SpeedDisplayString;

	/** The current gear as a string (R,N, 1,2 etc) */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
	FText GearDisplayString;

	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
	/** The color of the incar gear text in forward gears */
	FColor	GearDisplayColor;

	/** The color of the incar gear text when in reverse */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
	FColor	GearDisplayReverseColor;

	/** Are we using incar camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly)
	bool bInCarCameraActive;

	/** Are we in reverse gear */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly)
	bool bInReverseGear;

	/** Initial offset of incar camera */
	FVector InternalCameraOrigin;

	// Begin Pawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End Pawn interface

	// Begin Actor interface
	virtual void Tick(float Delta) override;

	/** Returns Mesh subobject **/
	FORCEINLINE UStaticMeshComponent* ASkyboundPawn::GetMesh() const { return Mesh; }
protected:
	virtual void BeginPlay() override;

public:
	// End Actor interface

	/* In meters per second */
	UPROPERTY(Category = Flight, EditAnywhere)
	float InitialVelocity = 1120.0f;

	/* Engine force applied via ActorForwardVector per second = PlaneMass * BaseEngineAcceleration */
	UPROPERTY(Category = Flight, EditAnywhere)
	float BaseEngineAcceleration = 20.0f;

	/* Drag Force applied against horizontal velocity per second: DragCoefficient * 0.5f * AirDensity * HorizontalVelocityMagnitudeSquared * WingArea */
	UPROPERTY(Category = Flight, EditAnywhere)
	float DragCoefficient = 0.0f;

	/* Lift Force applied via ActorUpVector per second: LiftCoefficient * 0.5f * AirDensity * HorizontalVelocityMagnitudeSquared * WingArea */
	UPROPERTY(Category = Flight, EditAnywhere)
	float LiftCoefficient = 0.01f;

	/* Number of degrees to add to the actual AoA. Can be used to represent wings that are angled in relation to the plane body. */
	UPROPERTY(Category = Flight, EditAnywhere)
	float AOAOffset = 0.f;

	/* Multiplied by the actual AoA to create lift due to AoA. Higher AoAScaling means more drastic velocity changes when pitching the plane up and down. */
	UPROPERTY(Category = Flight, EditAnywhere)
	float AOAScaling = 0.14f;

	/* Scales linearly with Lift/Drag */
	UPROPERTY(Category = Flight, EditAnywhere)
	float AirDensity = 1.0f;

	/* Scales linearly with Lift/Drag */
	UPROPERTY(Category = Flight, EditAnywhere)
	float WingArea = 1.0f;

	/* Used for applying forces other than Lift/Drag */
	UPROPERTY(Category = Flight, EditAnywhere)
	float PlaneMass = 1000.0f;

	/* Pitch Torque per second = PlaneMass * PitchAccelerationPerFrame */
	UPROPERTY(Category = Flight, EditAnywhere)
	float PitchAccelerationPerFrame = 6.0f;

	/* Yaw Torque per second = PlaneMass * YawAccelerationPerFrame */
	UPROPERTY(Category = Flight, EditAnywhere)
	float YawAccelerationPerFrame = 6.0f;

	/* Roll Torque per second = PlaneMass * YawAccelerationPerFrame */
	UPROPERTY(Category = Flight, EditAnywhere)
	float RollAccelerationPerFrame = 6.0f;
	
	/****** INPUT BINDING FUNCTION*****/
	/** Handle Pitch input */
	void Pitch(float Val);

	/** Handle Yaw input */
	void Yaw(float Val);

	/** Handle Roll input */
	void Roll(float Val);

	/** Handle acceleration / deceleration */
	void Accelerate(float Val);

	/****** END INPUT BINDING *****/

	/** Setup the strings used on the hud */
	void SetupInCarHUD();

	/** Update the physics material used by the vehicle mesh */
	void UpdatePhysicsMaterial();

	/** Handle pressing right */
	void MoveRight(float Val);
	/** Handle handbrake pressed */
	void OnHandbrakePressed();
	/** Handle handbrake released */
	void OnHandbrakeReleased();
	/** Switch between cameras */
	void OnToggleCamera();
	/** Handle reset VR device */
	void OnResetVR();

	static const FName LookUpBinding;
	static const FName LookRightBinding;
	static const FName EngineAudioRPM;

private:

	/** Per frame data */
	float PitchMultiplierThisFrame;
	float YawMultiplierThisFrame;
	float RollMultiplierThisFrame;
	float AccelerationMultiplierThisFrame;

	void ResetFrameData();

	/** 
	 * Activate In-Car camera. Enable camera and sets visibility of incar hud display
	 *
	 * @param	bState true will enable in car view and set visibility of various
	 */
	void EnableIncarView( const bool bState );

	/** Update the gear and speed strings */
	void UpdateHUDStrings();

	/* Are we on a 'slippery' surface */
	bool bIsLowFriction;
	/** Slippery Material instance */
	UPhysicalMaterial* SlipperyMaterial;
	/** Non Slippery Material instance */
	UPhysicalMaterial* NonSlipperyMaterial;

	/** The main static mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mesh;

public:
	/** Returns SpringArm subobject **/
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm; }
	/** Returns Camera subobject **/
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }
	/** Returns InternalCamera subobject **/
	FORCEINLINE UCameraComponent* GetInternalCamera() const { return InternalCamera; }
	/** Returns InCarSpeed subobject **/
	FORCEINLINE UTextRenderComponent* GetInCarSpeed() const { return InCarSpeed; }
	/** Returns InCarGear subobject **/
	FORCEINLINE UTextRenderComponent* GetInCarGear() const { return InCarGear; }
	/** Returns EngineSoundComponent subobject **/
	FORCEINLINE UAudioComponent* GetEngineSoundComponent() const { return EngineSoundComponent; }
};
