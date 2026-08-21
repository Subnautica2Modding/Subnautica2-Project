#include "SuzieCustomSerializers.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace SuzieCustomSerializers
{
    static constexpr int32 SpatialCellSize = 3;

    static bool Serialize_UWEWorldPopSpatialLayer(FArchive& Ar, void* Data, UScriptStruct* Struct)
    {
        FMapProperty* MapProp = CastField<FMapProperty>(Struct->FindPropertyByName(TEXT("CellMap")));
        if (MapProp == nullptr) return false;

        FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(Data));

        if (Ar.IsLoading())
        {
            int32 Count = 0;
            Ar << Count;
            MapHelper.EmptyValues(Count);
            for (int32 EntryIndex = 0; EntryIndex < Count; ++EntryIndex)
            {
                int64 Key = 0;
                Ar << Key;
                uint8 CellBytes[SpatialCellSize] = {};
                Ar.Serialize(CellBytes, SpatialCellSize);

                const int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
                *reinterpret_cast<int64*>(MapHelper.GetKeyPtr(NewIndex)) = Key;
                FMemory::Memcpy(MapHelper.GetValuePtr(NewIndex), CellBytes, SpatialCellSize);
            }
            MapHelper.Rehash();
        }
        else
        {
            int32 Count = MapHelper.Num();
            Ar << Count;
            for (int32 SparseIndex = 0; SparseIndex < MapHelper.GetMaxIndex(); ++SparseIndex)
            {
                if (!MapHelper.IsValidIndex(SparseIndex)) continue;
                int64 Key = *reinterpret_cast<const int64*>(MapHelper.GetKeyPtr(SparseIndex));
                Ar << Key;
                uint8 CellBytes[SpatialCellSize] = {};
                FMemory::Memcpy(CellBytes, MapHelper.GetValuePtr(SparseIndex), SpatialCellSize);
                Ar.Serialize(CellBytes, SpatialCellSize);
            }
        }
        return true;
    }
    
    static bool SerializeRawPOD(FArchive& Ar, void* Data, UScriptStruct* Struct, int32 GameSize)
    {
        const int32 EditorSize = Struct->GetStructureSize();
        if (EditorSize == GameSize)
        {
            Ar.Serialize(Data, GameSize);
            return true;
        }

        TArray<uint8, TInlineAllocator<16>> Temp;
        Temp.SetNumZeroed(GameSize);
        const int32 Overlap = FMath::Min(EditorSize, GameSize);
        if (Ar.IsLoading())
        {
            Ar.Serialize(Temp.GetData(), GameSize);
            FMemory::Memcpy(Data, Temp.GetData(), Overlap);
        }
        else
        {
            FMemory::Memcpy(Temp.GetData(), Data, Overlap);
            Ar.Serialize(Temp.GetData(), GameSize);
        }
        return true;
    }

    static bool Serialize_MercunaUsageTypes(FArchive& Ar, void* Data, UScriptStruct* Struct)
    {
        return SerializeRawPOD(Ar, Data, Struct, 4);
    }

    static bool Serialize_MercunaUsageSpec(FArchive& Ar, void* Data, UScriptStruct* Struct)
    {
        return SerializeRawPOD(Ar, Data, Struct, 8);
    }

    FStructSerializeFn FindStructSerializer(const FString& StructPath)
    {
        if (StructPath == TEXT("/Script/UWEWorldPopulation2.UWEWorldPopSpatialLayer"))
        {
            return &Serialize_UWEWorldPopSpatialLayer;
        }
        if (StructPath == TEXT("/Script/Mercuna.MercunaUsageTypes"))
        {
            return &Serialize_MercunaUsageTypes;
        }
        if (StructPath == TEXT("/Script/Mercuna.MercunaUsageSpec"))
        {
            return &Serialize_MercunaUsageSpec;
        }
        return nullptr;
    }
}
