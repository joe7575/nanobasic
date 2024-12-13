package = "nanobasic"
version = "1.0-0"
source = {
    url = "git+https://github.com/joe7575/nanobasic.git"
}

description = {
    summary = "A small BASIC compiler with virtual machine for Luanti",
    detailed = [[
        tbd.
    ]],
    homepage = "https://github.com/joe7575/nanobasic",
    license = "MIT"
}

dependencies = {
   "lua == 5.1"
}

build = {
    type = "builtin",
    modules = {
        nanobasiclib = {
            "./nb_lua.c",
            "./nb_scanner.c",
            "./nb_compiler.c",
            "./nb_runtime.c",
            "./nb_memory.c"
        },
    }
}
