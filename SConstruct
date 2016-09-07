
import json
import os
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
    ("CLANGTIDY", "Clang Tidy", "clang-tidy38"),
    EnumVariable("VERBOSE", "Show full build information", "0", ["0", "1"]),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsc", "tsx"]),
    EnumVariable("CFG", "R/R Log Config", "ft", ["ft", "dbg", "snap"])
)

objlib = Builder(action = Action('ld -r -o $TARGET $SOURCES','$OBJLIBCOMSTR'),
                 suffix = '.o',
                 src_suffix = '.c',
                 src_builder = 'StaticObject')

def clangtidy(target, source, env):
    try:
        f = open("compile_commands.json")
        j = json.load(f)
        for x in j:
            result = os.system(env["CLANGTIDY"]+" "+x["file"])
    except IOError as e:
        return []

env = Environment(options = opts, tools = ['default', 'compilation_db'],
        BUILDERS = {'ObjectLibrary' : objlib})
Help("""TARGETS:
scons               Build castor
scons sysroot       Build sysroot
scons test          Run tests
scons compiledb     Compile Database
scons check         Clang tidy checker\n""")
Help(opts.GenerateHelpText(env))

# Clang scan-build support
env["CC"] = os.getenv("CC") or env["CC"]
env["CXX"] = os.getenv("CXX") or env["CXX"]
env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

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

env["SYSROOT"] = os.getcwd() + "/sysroot/usr/amd64-freebsd/"

Export('env')


VariantDir("build/lib", "lib")
SConscript("#build/lib/Runtime/SConstruct")
SConscript("#build/lib/Checkpointing/SConstruct")
SConscript("#build/lib/Common/SConstruct")

VariantDir("build/tools", "tools")
SConscript("#build/tools/rrlog/SConstruct")
SConscript("#build/tools/record/SConstruct")

VariantDir("build/test", "test")
SConscript("#build/test/SConstruct")

VariantDir("build/perf", "perf")
SConscript("#build/perf/SConstruct")

AlwaysBuild(Alias('test', "build/librr/librr.o", "test/testbench.py"))
AlwaysBuild(Alias('sysroot', "", "tools/sysroot.sh"))

compileDb = env.Alias("compiledb", env.CompilationDatabase('compile_commands.json'))
if ("check" in BUILD_TARGETS):
    Alias('check', env.Command("fake_clang_tidy_target",
                               "compile_commands.json", clangtidy))

