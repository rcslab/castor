
import multiprocessing

opts = Variables('Local.sc')
opts.AddVariables(
    EnumVariable("BUILDTYPE", "Build Type", "DEBUG", ["DEBUG", "RELEASE"]),
    ("CC", "C Compiler", "cc"),
    ("CXX", "C++ Compiler", "c++"),
    ("AS", "Assembler", "as"),
    ("LINK", "Linker", "cc"),
    ("AR", "Archiver", "ar"),
    ("RANLIB", "Archiver Indexer", "ranlib"),
    ("NUMCPUS", "Number of CPUs to use for build (0 means auto)", "0"),
    EnumVariable("VERBOSE", "Show full build information", "0", ["0", "1"]),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsc", "tsx"]),
    EnumVariable("CFG", "R/R Log Config", "ft", ["ft", "dbg", "snap"])
)

objlib = Builder(action = Action('ld -r -o $TARGET $SOURCES','$OBJLIBCOMSTR'),
                 suffix = '.o',
                 src_suffix = '.c',
                 src_builder = 'StaticObject')

env = Environment(options = opts, BUILDERS = {'ObjectLibrary' : objlib})
Help(opts.GenerateHelpText(env))

if env["VERBOSE"] == "0":
    env["CCCOMSTR"] = "Compiling $SOURCE"
    env["CXXCOMSTR"] = "Compiling $SOURCE"
    env["SHCCCOMSTR"] = "Compiling $SOURCE"
    env["SHCXXCOMSTR"] = "Compiling $SOURCE"
    env["LINKCOMSTR"] = "Linking $TARGET"
    env["SHLINKCOMSTR"] = "Linking $TARGET"
    env["OBJLIBCOMSTR"] = "Linking $TARGET"
    env["ASCOMSTR"] = "Assembling $TARGET"
    env["ASPPCOMSTR"] = "Assembling $TARGET"
    env["ARCOMSTR"] = "Creating library $TARGET"
    env["RANLIBCOMSTR"] = "Indexing library $TARGET"

def GetNumCPUs(env):
    if env["NUMCPUS"] != "0":
        return int(env["NUMCPUS"])
    return 2*multiprocessing.cpu_count()

env.SetOption('num_jobs', GetNumCPUs(env))

env.Append(CXXFLAGS = ["-std=c++11"])
env.Append(CPPFLAGS = ["-Wall", "-Werror", "-g", "-O2"])

if (env["BUILDTYPE"] == "DEBUG"):
    env.Append(CPPFLAGS = ["-DCASTOR_DEBUG"])
elif (env["BUILDTYPE"] == "RELEASE"):
    env.Append(CPPFLAGS = ["-DCASTOR_RELEASE"])
else:
    print "Unknown BUILDTYPE"

if (env["RR"] == "ctr"):
    env.Append(CPPFLAGS = ["-DCASTOR_CTR"])
elif (env["RR"] == "tsc"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSC"])
elif (env["RR"] == "tsx"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSX"])
else:
    print "Unknown RR"

if (env["CFG"] == "ft"):
    env.Append(CPPFLAGS = ["-DCASTOR_FT"])
elif (env["CFG"] == "dbg"):
    env.Append(CPPFLAGS = ["-DCASTOR_DBG"])
elif (env["CFG"] == "snap"):
    env.Append(CPPFLAGS = ["-DCASTOR_SNAP"])
else:
    print "Unknown CFG"

env.Append(CPPPATH = ["#include", "#include/" + env["RR"]])

Export('env')

SConscript("librr/SConstruct", variant_dir="build/librr")
SConscript("libsnap/SConstruct", variant_dir="build/libsnap")
SConscript("rrlog/SConstruct", variant_dir="build/rrlog")
SConscript("record/SConstruct", variant_dir="build/record")
SConscript("test/SConstruct", variant_dir="build/test")

AlwaysBuild(Alias('test', "build/librr/librr.o", "test/testbench.py"))

