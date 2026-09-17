#include "ALDropship.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
AALDropship::AALDropship()
{
	PrimaryActorTick.bCanEverTick = true; bReplicates = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); SetRootComponent(Root);
}
void AALDropship::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(FVector(-PathHalf, 0.f, Altitude));
	if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this);
		Body->SetStaticMesh(Cube); Body->SetupAttachment(Root);
		Body->SetRelativeScale3D(FVector(18.f,6.f,2.2f)); Body->RegisterComponent();
	}
}
void AALDropship::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Travel += Speed * DeltaSeconds;
	SetActorLocation(FVector(FMath::Clamp(-PathHalf + Travel, -PathHalf, PathHalf), 0.f, Altitude));
}
