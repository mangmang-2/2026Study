#include "IconGeneratorHelper.h"

#if WITH_EDITOR
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

TArray<FString> UIconGeneratorHelper::GetMeshAssetsInFolder(
    const FString& FolderPath, EIconMeshType MeshType)
{
    TArray<FString> Result;

#if WITH_EDITOR
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    const UClass* MeshClass = (MeshType == EIconMeshType::Skeletal)
        ? USkeletalMesh::StaticClass()
        : UStaticMesh::StaticClass();

    TArray<FAssetData> Assets;
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*FolderPath));
    Filter.ClassPaths.Add(MeshClass->GetClassPathName());
    Filter.bRecursivePaths = true;

    Registry.GetAssets(Filter, Assets);

    for (const FAssetData& Asset : Assets)
        Result.Add(Asset.GetObjectPathString());
#endif

    return Result;
}

FString UIconGeneratorHelper::GetAssetNameFromPath(const FString& AssetPath)
{
    // "/Game/Meshes/Weapons/SK_Sword01.SK_Sword01" or "/Game/Meshes/SK_Sword01"
    FString Left, Right;
    if (AssetPath.Split(TEXT("."), &Left, &Right))
        return FPackageName::GetShortName(Left);
    return FPackageName::GetShortName(AssetPath);
}

UTexture2D* UIconGeneratorHelper::CaptureAndSaveIcon(
    UTextureRenderTarget2D* RenderTarget,
    const FString& AssetName,
    const FString& SavePath)
{
#if WITH_EDITOR
    if (!RenderTarget || AssetName.IsEmpty()) return nullptr;

    const FString PackageName = SavePath / AssetName;
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package) return nullptr;

    UTexture2D* Texture = RenderTarget->ConstructTexture2D(
        Package, *AssetName, RF_Public | RF_Standalone);
    if (!Texture) return nullptr;

    Texture->PostEditChange();
    FAssetRegistryModule::AssetCreated(Texture);
    Package->MarkPackageDirty();

    const FString FilePath = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Texture, *FilePath, SaveArgs);

    return Texture;
#else
    return nullptr;
#endif
}

USkeletalMesh* UIconGeneratorHelper::LoadSkeletalMesh(const FString& AssetPath)
{
#if WITH_EDITOR
    return Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, *AssetPath));
#else
    return nullptr;
#endif
}

UStaticMesh* UIconGeneratorHelper::LoadStaticMesh(const FString& AssetPath)
{
#if WITH_EDITOR
    return Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *AssetPath));
#else
    return nullptr;
#endif
}
