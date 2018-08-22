// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SkyboundPawn.h"
#include "SkyboundWheelFront.h"
#include "SkyboundWheelRear.h"
#include "SkyboundHud.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "UObject/ConstructorHelpers.h"

// Needed for VR Headset
#if HMD_MODULE_INCLUDED
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#endif // HMD_MODULE_INCLUDED

DEFINE_LOG_CATEGORY_STATIC(LogSkyboundPawn, Log, All);

const FName ASkyboundPawn::LookUpBinding("LookUp");
const FName ASkyboundPawn::LookRightBinding("LookRight");
const FName ASkyboundPawn::EngineAudioRPM("RPM");

#define LOCTEXT_NAMESPACE "VehiclePawn"

ASkyboundPawn::ASkyboundPawn()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyboundMeshComponent"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Mesh->BodyInstance.bSimulatePhysics = true;
	RootComponent = Mesh;

	// Car mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CarMesh(TEXT("/Game/Geometry/Meshes/PlaneTest.PlaneTest"));
	
	if (GetMesh())
	{
		GetMesh()->SetStaticMesh(CarMesh.Object);
		GetMesh()->SetRelativeScale3D(FVector(3.0f, 1.0f, 1.0f));
	}

// 	static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(TEXT("/Game/VehicleAdv/Vehicle/VehicleAnimationBlueprint"));
// 	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
// 	GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);

	// Setup friction materials
	static ConstructorHelpers::FObjectFinder<UPhysicalMaterial> SlipperyMat(TEXT("/Game/VehicleAdv/PhysicsMaterials/Slippery.Slippery"));
	SlipperyMaterial = SlipperyMat.Object;
		
	static ConstructorHelpers::FObjectFinder<UPhysicalMaterial> NonSlipperyMat(TEXT("/Game/VehicleAdv/PhysicsMaterials/NonSlippery.NonSlippery"));
	NonSlipperyMaterial = NonSlipperyMat.Object;

	// Create a spring arm component for our chase camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetRelativeLocation(FVector(-750.0f, 0.0f, 100.0f));
	SpringArm->SetWorldRotation(FRotator(-20.0f, 0.0f, 0.0f));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 125.0f;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = true;

	// Create the chase camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetRelativeLocation(FVector(-125.0, 0.0f, 0.0f));
	Camera->SetRelativeRotation(FRotator(10.0f, 0.0f, 0.0f));
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;

	// Create In-Car camera component 
	InternalCameraOrigin = FVector(-34.0f, -10.0f, 50.0f);
	InternalCameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("InternalCameraBase"));
	InternalCameraBase->SetRelativeLocation(InternalCameraOrigin);
	InternalCameraBase->SetupAttachment(GetMesh());

	InternalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InternalCamera"));
	InternalCamera->bUsePawnControlRotation = false;
	InternalCamera->FieldOfView = 90.f;
	InternalCamera->SetupAttachment(InternalCameraBase);

	// In car HUD
	// Create text render component for in car speed display
	InCarSpeed = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarSpeed"));
	InCarSpeed->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
	InCarSpeed->SetRelativeLocation(FVector(35.0f, -6.0f, 20.0f));
	InCarSpeed->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	InCarSpeed->SetupAttachment(GetMesh());

	// Create text render component for in car gear display
	InCarGear = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
	InCarGear->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));
	InCarGear->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
	InCarGear->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	InCarGear->SetupAttachment(GetMesh());
	
	// Setup the audio component and allocate it a sound cue
	static ConstructorHelpers::FObjectFinder<USoundCue> SoundCue(TEXT("/Game/VehicleAdv/Sound/Engine_Loop_Cue.Engine_Loop_Cue"));
	EngineSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineSound"));
	EngineSoundComponent->SetSound(SoundCue.Object);
	EngineSoundComponent->SetupAttachment(GetMesh());

	// Colors for the in-car gear display. One for normal one for reverse
	GearDisplayReverseColor = FColor(255, 0, 0, 255);
	GearDisplayColor = FColor(255, 255, 255, 255);

	bIsLowFriction = false;
	bInReverseGear = false;
}

void ASkyboundPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Pitch", this, &ASkyboundPawn::Pitch);
	PlayerInputComponent->BindAxis("Yaw", this, &ASkyboundPawn::Yaw);
	PlayerInputComponent->BindAxis("Roll", this, &ASkyboundPawn::Roll);
	PlayerInputComponent->BindAxis("Accelerate", this, &ASkyboundPawn::Accelerate);

	PlayerInputComponent->BindAction("SwitchCamera", IE_Pressed, this, &ASkyboundPawn::OnToggleCamera);

}

void ASkyboundPawn::Pitch(float Val)
{
	PitchMultiplierThisFrame = Val;
}

void ASkyboundPawn::Yaw(float Val)
{
	YawMultiplierThisFrame = Val;
}

void ASkyboundPawn::Roll(float Val)
{
	RollMultiplierThisFrame = Val;
}

void ASkyboundPawn::Accelerate(float Val)
{
	AccelerationMultiplierThisFrame = Val;
}

void ASkyboundPawn::MoveRight(float Val)
{

}

void ASkyboundPawn::OnHandbrakePressed()
{

}

void ASkyboundPawn::OnHandbrakeReleased()
{

}

void ASkyboundPawn::OnToggleCamera()
{
	EnableIncarView(!bInCarCameraActive);
}

void ASkyboundPawn::EnableIncarView(const bool bState)
{
	if (bState != bInCarCameraActive)
	{
		bInCarCameraActive = bState;
		
		if (bState == true)
		{
			OnResetVR();
			Camera->Deactivate();
			InternalCamera->Activate();
		}
		else
		{
			InternalCamera->Deactivate();
			Camera->Activate();
		}
		
		InCarSpeed->SetVisibility(bInCarCameraActive);
		InCarGear->SetVisibility(bInCarCameraActive);
	}
}

void ASkyboundPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	FVector Forward = GetActorForwardVector();
	FVector Right = GetActorRightVector();
	FVector Up = GetActorUpVector();

	Forward = Forward.RotateAngleAxis(-AOAOffset, Right);

	// Get velocity direction on the Up/Forward axes only
	FVector NormalizedVelocity = GetVelocity();
	NormalizedVelocity.ProjectOnToNormal(Right);
	FVector::VectorPlaneProject(NormalizedVelocity, Right);
	NormalizedVelocity.Normalize();

	float AOA = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(NormalizedVelocity, Forward)));

	float UpToForwardDot = FVector::DotProduct(Up, Forward); // This will always end up being 0
	float UpToVelocityDot = FVector::DotProduct(Up, NormalizedVelocity);
	
	// If the plane's nose is pointing higher than the direction it's traveling in, that's a positive AoA
	bool positiveAOA = UpToForwardDot >= UpToVelocityDot;
	if (!positiveAOA)
	{
		AOA *= -1.f;
	}

	float DynamicLiftCoefficient = LiftCoefficient * AOA * AOAScaling;

	// Overall velocity projected on the forward vector to get forward velocity
	float VelocityMagnitudeSquared = GetVelocity().SizeSquared();
	float LiftAndDragScalingValue = 0.5f * AirDensity * VelocityMagnitudeSquared * WingArea;
	
	// Upward lift force caused by forward motion
	GetMesh()->AddForce(Delta * DynamicLiftCoefficient * Up * LiftAndDragScalingValue, NAME_None, true);

	// Backward drag force cause by forward motion
	GetMesh()->AddForce(Delta * DragCoefficient * -NormalizedVelocity * LiftAndDragScalingValue, NAME_None, true);

	// Forward acceleration caused by engine (and backwards acceleration caused by brakes)
	if (AccelerationMultiplierThisFrame != 0.f)
	{
		GetMesh()->AddForce(Delta * PlaneMass * BaseEngineAcceleration * AccelerationMultiplierThisFrame * Forward, NAME_None, true);
	}

	// Rotational forces caused by... inputs
	if (PitchMultiplierThisFrame != 0.f)
	{
		GetMesh()->AddTorqueInDegrees(Delta * PlaneMass * PitchAccelerationPerFrame * PitchMultiplierThisFrame * Right, NAME_None, true);
	}
	if (YawMultiplierThisFrame != 0.f)
	{
		GetMesh()->AddTorqueInDegrees(Delta * PlaneMass * YawAccelerationPerFrame * YawMultiplierThisFrame * Up, NAME_None, true);
	}
	if (RollMultiplierThisFrame != 0.f)
	{
		GetMesh()->AddTorqueInDegrees(Delta * PlaneMass * RollAccelerationPerFrame * RollMultiplierThisFrame * Forward, NAME_None, true);
	}
	
	// Update phsyics material
	UpdatePhysicsMaterial();

	// Update the strings used in the hud (incar and onscreen)
	UpdateHUDStrings();

	// Set the string in the incar hud
	SetupInCarHUD();

	bool bHMDActive = false;
#if HMD_MODULE_INCLUDED
	if ((GEngine->XRSystem.IsValid() == true ) && ( (GEngine->XRSystem->IsHeadTrackingAllowed() == true) || (GEngine->IsStereoscopic3D() == true)))
	{
		bHMDActive = true;
	}
#endif // HMD_MODULE_INCLUDED
	if( bHMDActive == false )
	{
		if ( (InputComponent) && (bInCarCameraActive == true ))
		{
			FRotator HeadRotation = InternalCamera->RelativeRotation;
			HeadRotation.Pitch += InputComponent->GetAxisValue(LookUpBinding);
			HeadRotation.Yaw += InputComponent->GetAxisValue(LookRightBinding);
			InternalCamera->RelativeRotation = HeadRotation;
		}
	}	

	ResetFrameData();
}

void ASkyboundPawn::BeginPlay()
{
	Super::BeginPlay();

	bool bWantInCar = false;
	// First disable both speed/gear displays 
	bInCarCameraActive = false;
	InCarSpeed->SetVisibility(bInCarCameraActive);
	InCarGear->SetVisibility(bInCarCameraActive);

	// Enable in car view if HMD is attached
#if HMD_MODULE_INCLUDED
	bWantInCar = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
#endif // HMD_MODULE_INCLUDED

	EnableIncarView(bWantInCar);
	// Start an engine sound playing
	EngineSoundComponent->Play();

	GetMesh()->AddImpulse(InitialVelocity * GetActorForwardVector(), NAME_None, true);
}

void ASkyboundPawn::OnResetVR()
{
#if HMD_MODULE_INCLUDED
	if (GEngine->XRSystem.IsValid())
	{
		GEngine->XRSystem->ResetOrientationAndPosition();
		InternalCamera->SetRelativeLocation(InternalCameraOrigin);
		GetController()->SetControlRotation(FRotator());
	}
#endif // HMD_MODULE_INCLUDED
}

void ASkyboundPawn::UpdateHUDStrings()
{
	//TODO: Get acutal plane speed
	float KPH = 0;//FMath::Abs(GetVehicleMovement()->GetForwardSpeed()) * 0.036f;
	int32 KPH_int = FMath::FloorToInt(KPH);
	//TODO: Rip out concept of gears
	int32 Gear = 0;// GetVehicleMovement()->GetCurrentGear();

	// Using FText because this is display text that should be localizable
	SpeedDisplayString = FText::Format(LOCTEXT("SpeedFormat", "{0} km/h"), FText::AsNumber(KPH_int));


	if (bInReverseGear == true)
	{
		GearDisplayString = FText(LOCTEXT("ReverseGear", "R"));
	}
	else
	{
		GearDisplayString = (Gear == 0) ? LOCTEXT("N", "N") : FText::AsNumber(Gear);
	}

}

void ASkyboundPawn::SetupInCarHUD()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if ((PlayerController != nullptr) && (InCarSpeed != nullptr) && (InCarGear != nullptr))
	{
		// Setup the text render component strings
		InCarSpeed->SetText(SpeedDisplayString);
		InCarGear->SetText(GearDisplayString);
		
		if (bInReverseGear == false)
		{
			InCarGear->SetTextRenderColor(GearDisplayColor);
		}
		else
		{
			InCarGear->SetTextRenderColor(GearDisplayReverseColor);
		}
	}
}

void ASkyboundPawn::UpdatePhysicsMaterial()
{
	if (GetActorUpVector().Z < 0)
	{
		if (bIsLowFriction == true)
		{
			GetMesh()->SetPhysMaterialOverride(NonSlipperyMaterial);
			bIsLowFriction = false;
		}
		else
		{
			GetMesh()->SetPhysMaterialOverride(SlipperyMaterial);
			bIsLowFriction = true;
		}
	}
}

void ASkyboundPawn::ResetFrameData()
{
	PitchMultiplierThisFrame = 0.f;
	YawMultiplierThisFrame = 0.f;
	RollMultiplierThisFrame = 0.f;
	AccelerationMultiplierThisFrame = 0.f;
}


#undef LOCTEXT_NAMESPACE
