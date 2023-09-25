
import json
import os
import multiprocessing
import sys
import subprocess

log = None

def myspawn(sh, escape, cmd, args, env):
    fullCmd = [sh, '-c', ' '.join(args)]

    p = subprocess.Popen(fullCmd, env=env,
                         stderr=subprocess.PIPE, stdout=subprocess.PIPE)

    (stdout, stderr) = p.communicate()

    out = ""
    if p.returncode == 0:
        out += ' '.join(args)
        out += "\n"
        out += stdout.decode("ascii") + stderr.decode("ascii")
        sys.stdout.write(stdout + stderr)
        log.write(out)
    else:
        out += "----- Build Error -----\n"
        out += "Command: " + ' '.join(args) + '\n'
        out += "\nMessages:\n"
        out += stdout.decode("ascii") + stderr.decode("ascii")
        out += "-----------------------\n"
        sys.stdout.write(out)
        log.write(out)

    return p.returncode

opts = Variables('Local.sc')
opts.AddVariables(
    ("CC", "C Compiler", "cc"),
    ("CXX", "C++ Compiler", "c++"),
    ("AS", "Assembler", "as"),
    ("LINK", "Linker", "c++"),
    ("AR", "Archiver", "ar"),
    ("RANLIB", "Archiver Indexer", "ranlib"),
    ("TESTCC", "C Compiler", "llvm/build/bin/clang"),
    ("TESTCXX", "C++ Compiler", "llvm/build/bin/clang++"),
    ("TESTLINK", "Linker", "llvm/build/bin/clang"),
    ("NUMCPUS", "Number of CPUs to use for build (0 means auto)", "0"),
    ("CLANGTIDY", "Clang Tidy", "clang-tidy40"),
    ("CTAGS", "Ctags", "exctags"),
    ("LOGFILE", "Output build log to a file", ""),
    EnumVariable("CLANGSAN", "Clang/LLVM Sanitizer", "", ["", "address", "thread", "leak"]),
    ("CASTORPASS", "Castor LLVM IR Pass", "lib/Pass/libCastorPass.so"),
    EnumVariable("VERBOSE", "Show full build information", "0", ["0", "1"]),
    EnumVariable("COLOR", "Color build output (breaks vim quickfix).", "yes", ["yes", "no"]),
    EnumVariable("BUILDTYPE", "Build Type", "DEBUG", ["DEBUG", "RELEASE"]),
    EnumVariable("ARCH", "CPU Architecture", "amd64", ["amd64"]),
    EnumVariable("RR", "R/R Type", "ctr", ["ctr", "tsc", "tsx"]),
    EnumVariable("CFG", "R/R Log Config", "ft", ["ft", "dbg", "snap"]),
    PathVariable("PREFIX", "Installation target directory", "/usr/local", PathVariable.PathAccept),
    PathVariable("DESTDIR", "Root directory", "", PathVariable.PathAccept)
)

env = Environment(options = opts,
                  tools = ['default', 'compilation_db', 'objectlib',
                           'clangtidy', 'copytree'])
Help("""TARGETS:
scons               Build castor
scons sysroot       Build sysroot
scons llvm          Grab llvm tar balls, unpack them, build, the whole nine yards.
scons rebuild-llvm  Incremental rebuild of LLVM, handy if your hacking on the llvm runtime.
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
if env["LOGFILE"] != "":
    log = open(env["LOGFILE"], "w")
    env['SPAWN'] = myspawn
    # Force color output to play well with spawn overload
    if env["COLOR"] == "yes":
        env.Append(CPPFLAGS = ["-fcolor-diagnostics"])

if env["COLOR"] == "yes" and os.isatty(1) and os.isatty(2):
    env.Append(CPPFLAGS = ["-fcolor-diagnostics"])

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
#XXX we should only be disabling these warnings for Runtime/lib
env.Append(CPPFLAGS = ["-Wall", "-Wsign-compare", # "-Wsign-conversion",
                       "-Wcast-align", "-Werror", "-g", "-O2", "-fPIC"])

if (env["CLANGSAN"] != ""):
    env.Append(CPPFLAGS = ["-fsanitize=" + env["CLANGSAN"]])
    env.Append(LINKFLAGS = ["-fsanitize=" + env["CLANGSAN"]])

if (env["BUILDTYPE"] == "DEBUG"):
    env.Append(CPPFLAGS = ["-DCASTOR_DEBUG"])
elif (env["BUILDTYPE"] == "RELEASE"):
    env.Append(CPPFLAGS = ["-DCASTOR_RELEASE"])
else:
    print("Unknown BUILDTYPE")

if (env["RR"] == "ctr"):
    env.Append(CPPFLAGS = ["-DCASTOR_CTR"])
elif (env["RR"] == "tsc"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSC"])
elif (env["RR"] == "tsx"):
    env.Append(CPPFLAGS = ["-DCASTOR_TSX"])
else:
    print("Unknown RR")

if (env["CFG"] == "ft"):
    env.Append(CPPFLAGS = ["-DCASTOR_FT"])
elif (env["CFG"] == "dbg"):
    env.Append(CPPFLAGS = ["-DCASTOR_DBG"])
elif (env["CFG"] == "snap"):
    env.Append(CPPFLAGS = ["-DCASTOR_SNAP"])
else:
    print("Unknown CFG")

env.Append(CPPPATH = ["#include",
                      "#include/" + env["RR"],
                      "#include/" + env["ARCH"]])

env["SYSROOT"] = os.getcwd() + "/sysroot/"
env["BUILDROOT"] = os.getcwd() + "/lib/"

# Configuration
conf = env.Configure()

# XXX: Hack to support clang static analyzer
def CheckFailed(msg):
    print(msg)
    if os.getenv('CCC_ANALYZER_OUTPUT_FORMAT') != None:
        return
    Exit(1)

if not conf.CheckCC():
    CheckFailed("Your C compiler and/or environment is incorrectly configured.")
if not conf.CheckCXX():
    CheckFailed("Your C++ compiler and/or environment is incorrectly configured.")

if os.path.exists("llvm/build"):
    print("Using the custom LLVM build for tests.")
else:
    print("Using the system's LLVM 15")
    env["TESTCC"] = "clang15"
    env["TESTCXX"] = "clang15++"
    env["TESTLINK"] = "clang15"

if sys.platform != "freebsd13":
    print("Current Castor release is tested on FreeBSD 13.1")
    print("Running on another version requires updating the system call definitions")
    exit(1)

conf.Finish()

Export('env')

VariantDir("build/lib", "lib")
SConscript("#build/lib/Runtime/SConstruct")
SConscript("#build/lib/ThreadRuntime/SConstruct")
SConscript("#build/lib/Common/SConstruct")

VariantDir("build/tools", "tools")
SConscript("#build/tools/rrlog/SConstruct")
SConscript("#build/tools/record/SConstruct")
SConscript("#build/tools/replay/SConstruct")
SConscript("#build/tools/rrtool/SConstruct")

cp = env.Command("#lib/Pass/libCastorPass.so",
            [ "lib/Pass/CastorPass.cc", "lib/Pass/CastorPass.h",
              "lib/Pass/CMakeLists.txt" ],
            "cd lib/Pass && cmake . && cmake --build .")
env.Alias("CastorPass", "#lib/Pass/libCastorPass.so")

VariantDir("build/unit-tests", "unit-tests")
SConscript("#build/unit-tests/SConstruct")

VariantDir("build/perf", "perf")
SConscript("#build/perf/SConstruct")

AlwaysBuild(Alias('sysroot', "", "utils/sysroot.sh"))
AlwaysBuild(Alias('llvm', "", "utils/llvm.sh"))
AlwaysBuild(Alias('rebuild-llvm', "", "cd llvm/build && cmake --build ."))

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

