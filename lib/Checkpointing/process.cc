
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <iomanip>
#include <iostream>

using namespace std;

#define MEMREGION_PROT_R	0x4
#define MEMREGION_PROT_W	0x2
#define MEMREGION_PROT_X	0x1
#define PGSIZE			4096

#define MEMREGION_FLAG_NEW	1
#define MEMREGION_FLAG_REMOVE	2
#define MEMREGION_FLAG_DELTA	3

class MemoryRegion {
public:
    MemoryRegion(uint64_t s, uint64_t e, uint8_t p)
	: start(s), end(e), prot(p)
    {
	len = e - s;
	pages = len / PGSIZE;
    }
    ~MemoryRegion() {}
    uint64_t		start;
    uint64_t		end;
    uint64_t		len;
    uint64_t		pages;
    uint8_t		prot;
    uint8_t		flags;
/*    protection
	resident
	privateresident
	reference count
	shadowpagecount
	mappingflags
	tp*/
};

vector<string>
split(const string &str, char sep)
{
    size_t pos = 0;
    vector<string> rval;

    while (pos < str.length()) {
        size_t end = str.find(sep, pos);
        if (end == str.npos) {
            rval.push_back(str.substr(pos));
            break;
        }

        rval.push_back(str.substr(pos, end - pos));
        pos = end + 1;
    }

    return rval;
}

string
join(const vector<string> &str, char sep)
{
    unsigned int i;
    string rval = "";

    if (str.size() == 0)
        return rval;

    rval = str[0];
    for (i = 1; i < str.size(); i++) {
        rval += sep;
        rval += str[i];
    }

    return rval;
}

vector<MemoryRegion>
readMap(int pid)
{
    vector<MemoryRegion> regions;
    string path = "/proc/" + to_string(pid) + "/map";
    int mapfd = open(path.c_str(), O_RDONLY);
    char *buf = new char[4096];
    int len;

    len = read(mapfd, buf, 4096);
    string str(buf, len);

    vector<string> lines = split(str, '\n');
    for (const string &l : lines) {
	vector<string> fields = split(l, ' ');

	uint64_t start = stoull(fields[0], 0, 16);
	uint64_t end = stoull(fields[1], 0, 16);
	uint8_t prot = 0;
	if (fields[5][0] == 'r')
	    prot |= MEMREGION_PROT_R;
	if (fields[5][1] == 'w')
	    prot |= MEMREGION_PROT_W;
	if (fields[5][2] == 'x')
	    prot |= MEMREGION_PROT_X;

	if (prot != 0) {
	    regions.emplace_back(start, end, prot);
	}
    }

    close(mapfd);
    delete[] buf;

    return regions;
}

static pid_t lastSnapshot = -1;

void
createSnapshot()
{
    vector<MemoryRegion> oldRegions;
    vector<MemoryRegion> newRegions;
    vector<MemoryRegion> deltas;

    if (lastSnapshot != -1) {
	oldRegions = readMap(lastSnapshot);
    }
    newRegions = readMap(getpid());

    for (auto &n : newRegions) {
	/*
	 * Case I:
	 * N: <------>
	 * O:    <------>
	 *    FFF		Extended Front
	 *       DDDDD		Delta
	 *            RRR	Deleted Back
	 *
	 * Case II:
	 * N:    <------>
	 * O: <------>
	 *    RRR		Deleted Front
	 *       DDDDD		Delta
	 *	      BBB	Extended Back
	 */
	for (auto &o : oldRegions) {
	    // Case I
	    if (n.start < o.start && n.end > o.start) {
		// Extended Front
		MemoryRegion d = n;
		d.end = o.start;
		d.flags = MEMREGION_FLAG_NEW;
		deltas.push_back(d);
	    }
	    if (n.end < o.end && n.end > o.start) {
		// Deleted Back
		MemoryRegion d = o;
		d.start = n.end;
		d.flags = MEMREGION_FLAG_REMOVE;
		deltas.push_back(d);
	    }
	    // Case II
	    if (n.start > o.start && n.start < n.end) {
		// Deleted Front
		MemoryRegion d = o;
		d.end = n.start;
		d.flags = MEMREGION_FLAG_REMOVE;
		deltas.push_back(d);
	    }
	    if (n.end > o.end && n.start < o.end) {
		// Extended Back
		MemoryRegion d = n;
		d.start = o.end;
		d.flags = MEMREGION_FLAG_NEW;
		deltas.push_back(d);
	    }

	    // Overlapping Region
	    uint64_t start = (n.start > o.start) ? n.start : o.start;
	    uint64_t end = (n.end > o.end) ? o.end : n.end;
	    if (start < end) {
		MemoryRegion d = n;
		d.start = start;
		d.end = end;
		d.flags = MEMREGION_FLAG_DELTA;
		deltas.push_back(d);
	    }
	}
    }

    string path = "rrlog.mem";
    int datafd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);

    for (auto &d : deltas) {
	write(datafd, &d, sizeof(MemoryRegion));
    }
    for (auto &d : deltas) {
	if (d.flags == MEMREGION_FLAG_NEW) {
	    write(datafd, (void *)d.start, d.end - d.start);
	} else if ((d.flags == MEMREGION_FLAG_DELTA) && ((d.prot & MEMREGION_PROT_W) != 0)) {
	    write(datafd, (void *)d.start, d.end - d.start);
	}
    }

    close(datafd);

    cout << "SNAPSHOT" << endl;

    if (lastSnapshot != -1) {
	kill(lastSnapshot, SIGKILL);
    }

    while (1) {
	sleep(1);
    }
}

void
snapshot()
{
    pid_t next = fork();

    if (next == -1) {
	perror("fork: ");
	abort();
    }
    if (next == 0) {
	createSnapshot();
    }

    if (lastSnapshot != -1)
	waitpid(lastSnapshot, 0, 0);

    lastSnapshot = next;
}

int
main(int argc, const char *argv[])
{
    pid_t p = getpid();

    vector<MemoryRegion> regions = readMap(p);
    for (const MemoryRegion &r : regions) {
	cout << "Region: " << setbase(16) << r.start << " " << r.len << " " << (int)r.prot << endl;
    }

    for (int i = 0; i < 10; i++) {
	snapshot();
	sleep(1);
    }

    return 0;
}

