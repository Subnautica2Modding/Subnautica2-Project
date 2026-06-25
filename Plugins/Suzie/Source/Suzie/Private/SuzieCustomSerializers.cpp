#include "SuzieCustomSerializers.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace SuzieCustomSerializers
{
    // ---- /Script/UWEWorldPopulation2.UWEWorldPopSpatialLayer ----
    // Native layout is a single TMap<int64, FUWEWorldPopSpatialCell> CellMap, where the cell is a
    // 3-byte POD (three 1-byte, non-bitfield bools at offsets 0/1/2). The game's TMap native
    // serializer writes int32 ElementCount followed by ElementCount pairs of (int64 key, 3-byte
    // cell).
    static constexpr int32 SpatialCellSize = 3;

    static bool Serialize_UWEWorldPopSpatialLayer(FArchive& Ar, void* Data, UScriptStruct* Struct)
    {
        FMapProperty* MapProp = CastField<FMapProperty>(Struct->FindPropertyByName(TEXT("CellMap")));
        if (MapProp == nullptr)
        {
            // Schema unexpectedly changed; let default serialization try rather than corrupt blindly.
            return false;
        }

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
                if (!MapHelper.IsValidIndex(SparseIndex))
                {
                    continue;
                }
                int64 Key = *reinterpret_cast<const int64*>(MapHelper.GetKeyPtr(SparseIndex));
                Ar << Key;
                uint8 CellBytes[SpatialCellSize] = {};
                FMemory::Memcpy(CellBytes, MapHelper.GetValuePtr(SparseIndex), SpatialCellSize);
                Ar.Serialize(CellBytes, SpatialCellSize);
            }
        }
        return true;
    }

    // ---- Raw plain-old-data blob serializers ----
    // FMercunaUsageTypes is a 32-bit usage bitmask (4 bytes); FMercunaUsageSpec is two of them
    // (8 bytes). Both are STRUCT_Native | STRUCT_SerializeNative | STRUCT_CopyNative plain-old-data
    // bitfields, so the game's native serializer simply dumps the raw struct bytes. The Suzie-generated
    // reflected layout has the SAME width (verified from the jmap: properties_size 4 and 8), so a raw
    // byte copy of the game size round-trips correctly. GameSize is fixed from the jmap rather than read
    // from the live struct so stream consumption stays correct even if editor padding ever diverged.
    static bool SerializeRawPOD(FArchive& Ar, void* Data, UScriptStruct* Struct, int32 GameSize)
    {
        const int32 EditorSize = Struct->GetStructureSize();
        if (EditorSize == GameSize)
        {
            Ar.Serialize(Data, GameSize);
            return true;
        }

        // Sizes diverged: still consume exactly GameSize bytes from the stream, copying only the overlap.
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
