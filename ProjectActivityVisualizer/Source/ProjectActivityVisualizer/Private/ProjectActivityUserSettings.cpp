#include "ProjectActivityUserSettings.h"

FName UProjectActivityUserSettings::GetContainerName() const
{
    return FName(TEXT("Editor"));
}

FName UProjectActivityUserSettings::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}

FName UProjectActivityUserSettings::GetSectionName() const
{
    return FName(TEXT("ProjectActivity"));
}