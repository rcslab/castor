
#include <cstdint>

#include <string>
#include <vector>

/*
 * Three files:
 *  - process.snap - Snapshot index
 *  - process.mem - Memory dumps
 *  - process.log - Record/Replay log
 *
 */

using namespace std;

class Snapshot
{
    uint64_t		tsc;
    uint64_t		memOffset;
    uint64_t		logOffset;
    uint64_t		entries;
};

class SnapshotRegion
{
    uint64_t		vaddr;
    uint64_t		memOffset;
    uint64_t		length;
    uint64_t		_rsvd;
};

