
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
    ("CLANGTIDY", "Clang Tidy", "clang-tidy39"),
    ("CTAGS", "Ctags", "exctags"),
    EnumVariable("CLANGSAN", "Clang/LLVM Sanitizer", "", ["", "address", "thread", "leak"]),
    ("CASTORPASS", "Castor LLVM IR Pass", "lib/Pass/libCastorPass.so"),
    EnumVariable("VERBOSE", "Show full build information", "0", ["0", "1"]),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsc", "tsx"]),
    EnumVariable("CFG", "R/R Log Config", "ft", ["ft", "dbg", "snap"]),
    PathVariable("PREFIX", "Installation target directory", "/usr/local", PathVariable.PathAccept),
    PathVariable("DESTDIR", "Root directory", "", PathVariable.PathAccept)
)

env = Environment(options = opts,
                  tools = ['default', 'compilation_db', 'objectlib',
                           'clangtidy'])
Help("""TARGETS:
scons               Build castor
scons sysroot       Build sysroot
scons llvm          Build llvm
scons install       Install castor
scons testbench     Run tests
scons perfbench     Run performance tests
scons compiledb     Compile Database
scons check         Clang tidy checker
scons tags          Ctags\n""")
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
env.Append(CFLAGS = ["-std=c11"])
env.Append(CPPFLAGS = ["-Wall", "-Wsign-compare", "-Wsign-conversion",
                       "-Wcast-align", "-Werror", "-g", "-O2"])

if (env["CLANGSAN"] != ""):
    env.Append(CPPFLAGS = ["-fsanitize=" + env["CLANGSAN"]])
    env.Append(LINKFLAGS = ["-fsanitize=" + env["CLANGSAN"]])

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
SConscript("#build/tools/replay/SConstruct")
SConscript("#build/tools/baseline/SConstruct")
SConscript("#build/tools/cft/SConstruct")
SConscript("#build/tools/rrdump/SConstruct")
SConscript("#build/tools/rrtool/SConstruct")

cp = env.Command("#lib/Pass/libCastorPass.so",
            [ "lib/Pass/CastorPass.cc", "lib/Pass/CastorPass.h",
              "lib/Pass/CMakeLists.txt" ],
            "cd lib/Pass && cmake . && cmake --build .")
env.Alias("CastorPass", "#lib/Pass/libCastorPass.so")

VariantDir("build/test", "test")
SConscript("#build/test/SConstruct")

VariantDir("build/perf", "perf")
SConscript("#build/perf/SConstruct")

AlwaysBuild(Alias('sysroot', "", "utils/sysroot.sh"))
AlwaysBuild(Alias('llvm', "", "utils/llvm.sh"))

compileDb = env.Alias("compiledb", env.CompilationDatabase('compile_commands.json'))
if ("check" in BUILD_TARGETS):
    Alias('check', AlwaysBuild(env.ClangCheckAll(
            ["compile_commands.json", "#lib/Pass/compile_commands.json"])))

if ("tags" in BUILD_TARGETS):
    env.Command("tags", ["lib", "include", "tools"],
                              '$CTAGS -R -f $TARGET $SOURCES')

# Install
env.Install('$DESTDIR$PREFIX/bin','build/tools/record/record')
env.Install('$DESTDIR$PREFIX/bin','build/tools/replay/replay')
env.Install('$DESTDIR$PREFIX/bin','build/tools/rrlog/rrlog')
env.Install('$DESTDIR$PREFIX/bin','build/tools/rrtool/rrtool')
env.Install('$DESTDIR$PREFIX/lib','build/lib/Runtime/libCastorRuntime.o')
env.Install('$DESTDIR$PREFIX/lib','build/lib/Common/libCastorCommon.a')
env.Install('$DESTDIR$PREFIX/lib','build/lib/Pass/libCastorPass.so')
env.Install('$DESTDIR$PREFIX/include','include/castor')
env.Install('$DESTDIR$PREFIX/include/castor','include/' + env['RR'] + '/castor/rrlog.h')
env.Install('$DESTDIR$PREFIX/include/castor','include/' + env['RR'] + '/castor/rrplay.h')
env.Alias('install','$DESTDIR$PREFIX')

