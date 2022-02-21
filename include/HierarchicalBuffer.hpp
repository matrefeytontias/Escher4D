#ifndef INC_HIERARCHICAL_BUFFER
#define INC_HIERARCHICAL_BUFFER

/**
 * Describes the recursive hierarchical buffer structure used in real-time shadow volume computations.
 * Defined and used as in An Efficient Alias-free Shadow Algorithm for Opaque and
 * Transparent Objects using per-triangle Shadow Volumes, Sintorn, Olsson & Assarsson.
 * Hard-coded for 5 levels of 32-sized tiles, alternating between 8x4 and 4x8 for each level.
 */
namespace HierarchicalBuffer
{
    // Level-specific constants
    const int offsets[] = {
        0,
        8 * 4,
        8 * 4 + 32 * 32,
        8 * 4 + 32 * 32 + 256 * 128,
        8 * 4 + 32 * 32 + 256 * 128 + 1024 * 1024
    }, widths[] = {
        8,
        8 * 4,
        8 * 4 * 8,
        8 * 4 * 8 * 4,
        8 * 4 * 8 * 4 * 8
    }, heights[] = {
        4,
        4 * 8,
        4 * 8 * 4,
        4 * 8 * 4 * 8,
        4 * 8 * 4 * 8 * 4
    }, tileWidths[] = {
        8, 4, 8, 4, 8
    }, tileHeights[] = {
        4, 8, 4, 8, 4
    };
    
    /**
     * Returns a pointer on the subtile situated at (dx, dy) in the tile that ptr
     * points to at a given level.
     */
    inline float *getChild(float *ptr, float *bufferBase, int level, int dx, int dy)
    {
        return (&ptr[1] - bufferBase) * 32 + bufferBase + static_cast<uintptr_t>(dy * widths[level + 1] + dx);
    }
};


#endif
