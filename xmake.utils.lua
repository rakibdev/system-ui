function setup()
    set_languages("c++26")
    set_toolchains("clang")
    set_defaultmode("debug")
    add_rules("mode.debug", "mode.release")
    add_cxxflags("-Wno-absolute-value")
    
    if is_mode("debug") then add_defines("DEV") end
end

function set_build_dir()
    set_targetdir("build")
    -- before_build(function (target)
        -- os.rm(path.join(target:targetdir(), target:name()))
    -- end)
end

function set_extension()
    set_kind("shared")
    -- Fixes linking relocation for .so extensions.
    add_cxxflags("-fPIC")
    add_linkdirs("../../build/")
    add_links("system-ui")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "glaze")
end
