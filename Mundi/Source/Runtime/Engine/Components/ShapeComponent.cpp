#include "pch.h"
#include "ShapeComponent.h"

IMPLEMENT_CLASS(UShapeComponent)

void UShapeComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}

void UShapeComponent::OnSerialized()
{
    Super::OnSerialized();
}

bool UShapeComponent::Intersects(const UShapeComponent* A, const UShapeComponent* B)
{
	if (A->GetShapeType() == EShapeType::None || B->GetShapeType() == EShapeType::None) return false;

	return A->Intersects(B);
}