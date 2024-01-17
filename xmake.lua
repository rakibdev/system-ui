set_languages("cxx2b")
add_rules("mode.debug", "mode.release")
if is_mode("debug") then add_defines("DEV") end

add_requires("gtk+-3.0", {system = true})
add_requires("gtk-layer-shell-0", {system = true})
add_requires("libpipewire-0.3", {system = true})

target("glaze")
    set_kind("headeronly")
    add_headerfiles("libs/glaze/include/glaze/**.hpp")
    add_includedirs("libs/glaze/include", {public = true})

target("material-color-utilities")
    set_kind("object")
    add_files("libs/material-color-utilities/cpp/**.cc")
    add_includedirs("libs/material-color-utilities", {public = true})

target("system-ui")
    add_deps("glaze")
    add_deps("material-color-utilities")
    add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3")
    add_files("**.cpp|libs/**")