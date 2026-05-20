#include "NePythonSettings.h"

UNePythonSettings::UNePythonSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("NePythonBinding");

	bDisableOldStyleSubclassing = true;
}

#if WITH_EDITOR
FText UNePythonSettings::GetSectionText() const
{
	return FText::FromString(TEXT("NePythonBinding"));
}
#endif