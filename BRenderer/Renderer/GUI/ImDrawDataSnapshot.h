#ifndef BRR_IMDRAWDATASNAPSHOT_H
#define BRR_IMDRAWDATASNAPSHOT_H
// Usage:
//  static ImDrawDataSnapshot snapshot; // Important: make persistent accross frames to reuse buffers.
//  snapshot.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());
//  [...]
//  ImGui_ImplDX11_RenderDrawData(&snapshot.DrawData);

// FIXME: Could store an ID in ImDrawList to make this easier for user.
#include "imgui_internal.h" // ImPool<>, ImHashData

#include <utility>

struct ImDrawDataSnapshotEntry
{
    ImDrawList*     SrcCopy = NULL;     // Drawlist owned by main context
    ImDrawList*     OurCopy = NULL;     // Our copy
    double          LastUsedTime = 0.0;
};

struct ImDrawDataSnapshot
{
    // Members
    ImDrawData                      DrawData;
    ImPool<ImDrawDataSnapshotEntry> Cache;
    float                           MemoryCompactTimer = 20.0f; // Discard unused data after 20 seconds

    // Functions
    ImDrawDataSnapshot() = default;
    ImDrawDataSnapshot(ImDrawDataSnapshot&& other) noexcept;
    ~ImDrawDataSnapshot()           { Clear(); }

    ImDrawDataSnapshot& operator=(ImDrawDataSnapshot&& other) noexcept;

    void                            Clear();
    void                            SnapUsingSwap(ImDrawData* src, double current_time); // Efficient snapshot by swapping data, meaning "src_list" is unusable.
    //void                          SnapUsingCopy(ImDrawData* src, double current_time); // Deep-copy snapshop

    // Internals
    ImGuiID                         GetDrawListID(ImDrawList* src_list) { return ImHashData(&src_list, sizeof(src_list)); }     // Hash pointer
    ImDrawDataSnapshotEntry*        GetOrAddEntry(ImDrawList* src_list) { return Cache.GetOrAddByKey(GetDrawListID(src_list)); }
};

#endif