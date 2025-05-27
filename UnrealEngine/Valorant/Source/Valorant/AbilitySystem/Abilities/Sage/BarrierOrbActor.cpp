#include "BarrierOrbActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ABarrierOrbActor::ABarrierOrbActor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 루트 컴포넌트
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    
    // 오브 메시
    OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
    OrbMesh->SetupAttachment(GetRootComponent());
    OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // 기본 구체 메시 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        OrbMesh->SetStaticMesh(SphereMesh.Object);
        OrbMesh->SetRelativeScale3D(FVector(0.25f));
    }
    
    // 오브 이펙트
    OrbEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbEffect"));
    OrbEffect->SetupAttachment(OrbMesh);
    OrbEffect->bAutoActivate = true;
    
    // 포인트 라이트
    OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
    OrbLight->SetupAttachment(OrbMesh);
    OrbLight->SetIntensity(800.0f);
    OrbLight->SetAttenuationRadius(150.0f);
    OrbLight->SetLightColor(ValidColor);
    OrbLight->SetCastShadows(false);
}

void ABarrierOrbActor::BeginPlay()
{
    Super::BeginPlay();
    
    InitialRelativeLocation = OrbMesh->GetRelativeLocation();
    
    // 머티리얼 인스턴스 생성 및 설정
    if (OrbMesh->GetStaticMesh())
    {
        UMaterialInstanceDynamic* DynMaterial = OrbMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", ValidColor);
            DynMaterial->SetScalarParameterValue("EmissiveIntensity", 5.0f);
        }
    }
    
    // 아이들 사운드 재생
    if (OrbIdleSound)
    {
        IdleAudioComponent = UGameplayStatics::SpawnSoundAttached(
            OrbIdleSound, 
            OrbMesh, 
            NAME_None, 
            FVector::ZeroVector, 
            EAttachLocation::KeepRelativeOffset, 
            true
        );
    }
}

void ABarrierOrbActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 오브 회전
    FRotator CurrentRotation = OrbMesh->GetRelativeRotation();
    CurrentRotation.Yaw += OrbRotationSpeed * DeltaTime;
    OrbMesh->SetRelativeRotation(CurrentRotation);
    
    // 오브 상하 부유
    CurrentFloatTime += DeltaTime * OrbFloatSpeed;
    float FloatOffset = FMath::Sin(CurrentFloatTime) * OrbFloatHeight;
    FVector NewLocation = InitialRelativeLocation + FVector(0, 0, FloatOffset);
    OrbMesh->SetRelativeLocation(NewLocation);
}

void ABarrierOrbActor::SetPlacementValid(bool bValid)
{
    if (bIsPlacementValid == bValid)
        return;
    
    bIsPlacementValid = bValid;
    
    // 색상 변경
    FLinearColor TargetColor = bValid ? ValidColor : InvalidColor;
    
    // 라이트 색상 변경
    if (OrbLight)
    {
        OrbLight->SetLightColor(TargetColor);
    }
    
    // 머티리얼 색상 변경
    if (OrbMesh)
    {
        UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(OrbMesh->GetMaterial(0));
        if (DynMaterial)
        {
            DynMaterial->SetVectorParameterValue("EmissiveColor", TargetColor);
        }
    }
}