set_languages("c++latest")
set_defaultmode("debug")
add_rules("mode.debug", "mode.release")
if is_mode("debug") then add_defines("DEV") end

add_requires("system-ui", {system = true})
add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", {system = true})

-- todo: custom build/file.so rule.

-- option("extension")
--   set_kind("shared")
--   add_files("*.cpp")
--   add_packages("system-ui")

-- target("launcher")
--   add_options("extension")
--   add_files("protocols/*.c")

-- target("panel")
--   add_options("extension")

-- target("window-preview")
--   add_options("extension")

target("demo")
  set_kind("binary")
  add_files("demo/**.cpp")
  add_packages("system-ui")
  add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze")

  -- add_links("system-ui")
  -- add_linkdirs("/usr/lib")