// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ProjectActivitySettings.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_UOBJECT");
void EmptyLinkFunctionForGeneratedCodeProjectActivitySettings() {}

// ********** Begin Cross Module References ********************************************************
DEVELOPERSETTINGS_API UClass* Z_Construct_UClass_UDeveloperSettings(ETypeConstructPhase);
// ********** End Cross Module References **********************************************************

// ********** Begin Same Module References *********************************************************
UPackage* Z_Construct_UPackage__Script_ProjectActivityVisualizer(ETypeConstructPhase);
PROJECTACTIVITYVISUALIZER_API UScriptStruct* Z_Construct_UScriptStruct_FProjectActivityCategoryRule(ETypeConstructPhase);
PROJECTACTIVITYVISUALIZER_API UClass* Z_Construct_UClass_UProjectActivitySettings(ETypeConstructPhase);
PROJECTACTIVITYVISUALIZER_API UClass* Z_Construct_UClass_UProjectActivitySettings(ETypeConstructPhase);
// ********** End Same Module References ***********************************************************
#define UHT_STRUCT_BASE(INIT) UE::CodeGen::ConstInit::TCompiledInObjectPtr<const FStructBaseChain>(UE::Private::AsStructBaseChain(INIT))

// ********** Begin ScriptStruct FProjectActivityCategoryRule **************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UScriptStruct_FProjectActivityCategoryRule_Statics
struct UHT_STATICS
{
	static inline consteval int32 GetStructSize() { return DataSizeOf<FProjectActivityCategoryRule>(); }
	static inline consteval int16 GetStructAlignment() { return alignof(FProjectActivityCategoryRule); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xed\x8f\xb4\xeb\x8d\x94 \xea\xb2\xbd\xeb\xa1\x9c\xec\x99\x80 \xec\xb9\xb4\xed\x85\x8c\xea\xb3\xa0\xeb\xa6\xac\xeb\xa5\xbc \xec\x97\xb0\xea\xb2\xb0\xed\x95\x98\xeb\x8a\x94 \xea\xb7\x9c\xec\xb9\x99 \xed\x95\x98\xeb\x82\x98\n" },
#endif
		{ "ModuleRelativePath", "Public/ProjectActivitySettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xed\x8f\xb4\xeb\x8d\x94 \xea\xb2\xbd\xeb\xa1\x9c\xec\x99\x80 \xec\xb9\xb4\xed\x85\x8c\xea\xb3\xa0\xeb\xa6\xac\xeb\xa5\xbc \xec\x97\xb0\xea\xb2\xb0\xed\x95\x98\xeb\x8a\x94 \xea\xb7\x9c\xec\xb9\x99 \xed\x95\x98\xeb\x82\x98" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PathPrefix_MetaData[] = {
		{ "Category", "Category Rule" },
		{ "ModuleRelativePath", "Public/ProjectActivitySettings.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CategoryName_MetaData[] = {
		{ "Category", "Category Rule" },
		{ "ModuleRelativePath", "Public/ProjectActivitySettings.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FProjectActivityCategoryRule constinit property declarations ******
	static const UECodeGen_Private::FStrPropertyParams NewProp_PathPrefix;
	static const UECodeGen_Private::FNamePropertyParams NewProp_CategoryName;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FProjectActivityCategoryRule constinit property declarations ********
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FProjectActivityCategoryRule>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct UHT_STATICS

// ********** Begin ScriptStruct FProjectActivityCategoryRule Property Definitions *****************
const UECodeGen_Private::FStrPropertyParams UHT_STATICS::NewProp_PathPrefix = { "PathPrefix", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Str, nullptr, nullptr, 1, STRUCT_OFFSET(FProjectActivityCategoryRule, PathPrefix), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PathPrefix_MetaData), NewProp_PathPrefix_MetaData) };
const UECodeGen_Private::FNamePropertyParams UHT_STATICS::NewProp_CategoryName = { "CategoryName", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Name, nullptr, nullptr, 1, STRUCT_OFFSET(FProjectActivityCategoryRule, CategoryName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CategoryName_MetaData), NewProp_CategoryName_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_PathPrefix,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CategoryName,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End ScriptStruct FProjectActivityCategoryRule Property Definitions *******************
const UECodeGen_Private::FStructParams UHT_STATICS::StructParams = {
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_ProjectActivityVisualizer,
	nullptr,
	&NewStructOps,
	"ProjectActivityCategoryRule",
	UHT_STATICS::PropPointers,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	DataSizeOf<FProjectActivityCategoryRule>(),
	alignof(FProjectActivityCategoryRule),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule;
UScriptStruct* Z_Construct_UScriptStruct_FProjectActivityCategoryRule(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Outer)
	{
		if (!Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.OuterSingleton)
		{
			Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FProjectActivityCategoryRule, (UObject*)Z_Construct_UPackage__Script_ProjectActivityVisualizer(ETypeConstructPhase::Outer), TEXT("ProjectActivityCategoryRule"));
		}
		return Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.OuterSingleton;
	}
	if (!Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.InnerSingleton, UHT_STATICS::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule.InnerSingleton);
}
#undef UHT_STATICS
// ********** End ScriptStruct FProjectActivityCategoryRule ****************************************

// ********** Begin Class UProjectActivitySettings *************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_Construct_UClass_UProjectActivitySettings_Statics
struct UHT_STATICS
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Type_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xed\x94\x84\xeb\xa1\x9c\xec\xa0\x9d\xed\x8a\xb8\xeb\xb3\x84 \xed\x99\x9c\xeb\x8f\x99 \xeb\xb6\x84\xec\x84\x9d \xec\x84\xa4\xec\xa0\x95\n" },
#endif
		{ "DisplayName", "Project Activity" },
		{ "IncludePath", "ProjectActivitySettings.h" },
		{ "ModuleRelativePath", "Public/ProjectActivitySettings.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xed\x94\x84\xeb\xa1\x9c\xec\xa0\x9d\xed\x8a\xb8\xeb\xb3\x84 \xed\x99\x9c\xeb\x8f\x99 \xeb\xb6\x84\xec\x84\x9d \xec\x84\xa4\xec\xa0\x95" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CategoryRules_MetaData[] = {
		{ "Category", "Categories" },
		{ "ModuleRelativePath", "Public/ProjectActivitySettings.h" },
		{ "TitleProperty", "CategoryName" },
	};
#endif // WITH_METADATA

// ********** Begin Class UProjectActivitySettings constinit property declarations *****************
	static const UECodeGen_Private::FStructPropertyParams NewProp_CategoryRules_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_CategoryRules;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UProjectActivitySettings constinit property declarations *******************
	static FTypeConstructFunc* DependentSingletons[];
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UProjectActivitySettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct UHT_STATICS

// ********** Begin Class UProjectActivitySettings Property Definitions ****************************
const UECodeGen_Private::FStructPropertyParams UHT_STATICS::NewProp_CategoryRules_Inner = { "CategoryRules", nullptr, (EPropertyFlags)0x0000000000004000, UECodeGen_Private::EPropertyGenFlags::Struct, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FProjectActivityCategoryRule, METADATA_PARAMS(0, nullptr) }; // c06837bc939caf1c6224395f226edab091d24d17
const UECodeGen_Private::FArrayPropertyParams UHT_STATICS::NewProp_CategoryRules = { "CategoryRules", nullptr, (EPropertyFlags)0x0010000000004001, UECodeGen_Private::EPropertyGenFlags::Array, nullptr, nullptr, 1, STRUCT_OFFSET(UProjectActivitySettings, CategoryRules), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CategoryRules_MetaData), NewProp_CategoryRules_MetaData) }; // c06837bc939caf1c6224395f226edab091d24d17
const UECodeGen_Private::FPropertyParamsBase* const UHT_STATICS::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CategoryRules_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&UHT_STATICS::NewProp_CategoryRules,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::PropPointers) < 2048);
// ********** End Class UProjectActivitySettings Property Definitions ******************************
FTypeConstructFunc* UHT_STATICS::DependentSingletons[] = {
	(FTypeConstructFunc*)Z_Construct_UClass_UDeveloperSettings,
	(FTypeConstructFunc*)Z_Construct_UPackage__Script_ProjectActivityVisualizer,
};
static_assert(UE_ARRAY_COUNT(UHT_STATICS::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams UHT_STATICS::ClassParams = {
	&Z_Construct_UClass_UProjectActivitySettings,
	"Editor",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	UHT_STATICS::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(UHT_STATICS::PropPointers),
	0,
	0x001000A6u,
	METADATA_PARAMS(UE_ARRAY_COUNT(UHT_STATICS::Type_MetaData), UHT_STATICS::Type_MetaData)
};
FClassRegistrationInfo Z_Registration_Info_UClass_UProjectActivitySettings;
UClass* Z_Construct_UClass_UProjectActivitySettings(ETypeConstructPhase Phase)
{
	if (Phase == ETypeConstructPhase::Inner)
	{
		using TClass = UProjectActivitySettings;
		if (!Z_Registration_Info_UClass_UProjectActivitySettings.InnerSingleton)
		{
			GetPrivateStaticClassBody(
				TClass::StaticPackage(),
				TEXT("ProjectActivitySettings"),
				Z_Registration_Info_UClass_UProjectActivitySettings.InnerSingleton,
				nullptr,
				DataSizeOf<TClass>(),
				alignof(TClass),
				TClass::StaticClassFlags,
				TClass::StaticClassCastFlags(),
				TClass::StaticConfigName(),
				(UClass::ClassConstructorType)InternalConstructor<TClass>,
				(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
				UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
				&TClass::Super::StaticClass,
				&TClass::WithinClass::StaticClass
			);
		}
		return Z_Registration_Info_UClass_UProjectActivitySettings.InnerSingleton;
	}
	if (!Z_Registration_Info_UClass_UProjectActivitySettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UProjectActivitySettings.OuterSingleton, UHT_STATICS::ClassParams);
	}
	return Z_Registration_Info_UClass_UProjectActivitySettings.OuterSingleton;
}
#undef UHT_STATICS
UProjectActivitySettings::UProjectActivitySettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UProjectActivitySettings);
UProjectActivitySettings::~UProjectActivitySettings() {}
// ********** End Class UProjectActivitySettings ***************************************************

// ********** Begin Registration *******************************************************************
#ifdef UHT_STATICS
#error UHT_STATICS already defined
#endif
#define UHT_STATICS Z_CompiledInDeferFile_FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h__Script_ProjectActivityVisualizer_Statics
struct UHT_STATICS
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ Z_Construct_UScriptStruct_FProjectActivityCategoryRule, Z_Construct_UScriptStruct_FProjectActivityCategoryRule_Statics::NewStructOps, TEXT("ProjectActivityCategoryRule"),&Z_Registration_Info_UScriptStruct_FProjectActivityCategoryRule, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FProjectActivityCategoryRule), 3228055484U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UProjectActivitySettings, TEXT("UProjectActivitySettings"), &Z_Registration_Info_UClass_UProjectActivitySettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UProjectActivitySettings), 2183496356U) },
	};
}; // UHT_STATICS 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h__Script_ProjectActivityVisualizer_55fd39702b25c8ca8f78c9e19a64108d9dc4e0e5{
	TEXT("/Script/ProjectActivityVisualizer"),
	UHT_STATICS::ClassInfo, UE_ARRAY_COUNT(UHT_STATICS::ClassInfo),
	UHT_STATICS::ScriptStructInfo, UE_ARRAY_COUNT(UHT_STATICS::ScriptStructInfo),
	nullptr, 0,
	nullptr, 0,
};
#undef UHT_STATICS
// ********** End Registration *********************************************************************
#undef UHT_STRUCT_BASE

PRAGMA_ENABLE_DEPRECATION_WARNINGS
