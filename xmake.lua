-- Copyright Microsoft and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT

set_project("CHERIoT Hello World")
sdkdir = "/cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")
set_defaultmode("debug")

-- Support libraries
includes(path.join(sdkdir, "lib/freestanding"),
         path.join(sdkdir, "lib/string"),
         path.join(sdkdir, "lib/atomic"),
         path.join(sdkdir, "lib/microvium"),
         path.join(sdkdir, "lib/crt"))

option("board")
    set_default("ibex-safe-simulator")

compartment("main")
    add_files("main.cc")
    add_deps("user")

compartment("user")
    add_files("users.cc")
    

-- Firmware image for the example.
firmware("javascript")
    add_deps("crt", "freestanding", "string", "microvium", "atomic_fixed")
    add_deps("main")
    add_deps("user")
    add_deps("debug")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "main",
                priority = 1,
                entry_point = "run",
                stack_size = 0x800,
                trusted_stack_frames = 4
            }
        }, {expand = false})
    end)
