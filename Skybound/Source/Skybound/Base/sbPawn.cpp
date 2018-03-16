// Fill out your copyright notice in the Description page of Project Settings.

#include "sbPawn.h"


// Sets default values
AsbPawn::AsbPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AsbPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AsbPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AsbPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

