//
// Unit Test for JIT Porting
//

func main() {
    // Define an architecture list.
    arch_list = [
        {name: "Arm64",  tool: "aarch64-linux-gnu-",     qemu: "qemu-aarch64-static"},
	{name: "Arm32",  tool: "arm-linux-gnueabihf-",   qemu: "qemu-armhf-static"},
        {name: "MIPS64", tool: "mips64-linux-gnuabi64-", qemu: "qemu-mips64-static"},
        {name: "MIPS32", tool: "mips-linux-gnu-",        qemu: "qemu-mips-static"},
        {name: "PPC64",  tool: "powerpc64le-linux-gnu-", qemu: "qemu-ppc64el-static"},
        {name: "PPC32",  tool: "powerpc-linux-gnu-",     qemu: "qemu-ppc-static"},
        {name: "i386",   tool: "i686-linux-gnu-",        qemu: "qemu-i386-static"},
        {name: "x86_64", tool: "x86_64-linux-gnu-",      qemu: "qemu-x86_64-static"}
    ];

    // Call tests.
    for (arch in arch_list) {
        if (!arch_test(arch.name, arch.tool, arch.qemu)) {
	    return 1;
	}
    }

    return 0;
}

func arch_test(name, tool, qemu) {
    // Print a header.
    print("[" + name + "]");

    // Build a binary for a specified architecture.
    if (shell("cd .. && " +
              "make clean && " +
              "make CFLAGS=-static " +
                  "CC=" + tool + "gcc " +
                  "LD=" + tool + "ld " +
                  "AR=" + tool + "ar " +
                  "STRIP=" + tool + "strip " +
                  "linguine") != 0) {
        print("Build failed.");
	return 0;
    }

    // Copy a binary.
    shell("cp ../linguine ./linguine-arch");

    // Cleanup.
    shell("cd .. && make clean");

    // Run a testsuite.
    if (!run_testsuite(qemu)) {
        return 0;
    }

    return 1;
}

func run_testsuite(qemu) {
    testcase = [
        "syntax/01-assign.ls",
	"syntax/02-call.ls",
        "syntax/03-string.ls",
	"syntax/04-array.ls",
	"syntax/05-dictionary.ls",
	"syntax/06-for.ls",
	"syntax/07-lambda.ls",
	"syntax/08-double-loop.ls",
	"syntax/09-call-in-loop.ls",
	"syntax/10-if-elif-else.ls",
	"syntax/11-if-cond.ls",
	"syntax/12-elif-chain.ls",
		"syntax/13-call-args.ls"
    ];

    // Run tests without JIT.
    print("Interpreter...");
    for(file in testcase) {
        if (!run_testcase(file, qemu, "--disable-jit")) {
            return 0;
	}
    }

    // Run tests with JIT.
    print("JIT...");
    for(file in testcase) {
        if (!run_testcase(file, qemu, "")) {
	    return 0;
	}
    }

    return 1;
}

func run_testcase(file, qemu, option) {
    print("Running " + file + " ... ");

    // Run a testcase.
    if (shell(qemu + " ./linguine-arch " + option + " " + file + " > out") != 0) {
        print("Run failed.");
        return 0;
    }

    // Compare the result and the correct answer.
    if (shell("diff " + file + ".out out") != 0) {
        print("Diff.");
        return 0;
    }
	
    // Remove a temporary file.
    if (shell("rm -f out") != 0) {
        return 0;
    }

    return 1;
}
