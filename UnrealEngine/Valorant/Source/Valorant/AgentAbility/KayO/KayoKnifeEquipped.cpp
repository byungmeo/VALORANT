// Fill out your copyright notice in the Description page of Project Settings.


#include "KayoKnifeEquipped.h"


// Sets default values
AKayoKnifeEquipped::AKayoKnifeEquipped()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AKayoKnifeEquipped::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKayoKnifeEquipped::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

