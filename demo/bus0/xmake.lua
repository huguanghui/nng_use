target("bus0_t0")
    set_kind("binary")
    add_includedirs("$(nng_sdk)/include")
    add_linkdirs("$(nng_sdk)/lib")
    add_links("nng")
    add_syslinks("pthread", "m", "rt", "dl")
    add_files("t0.c")