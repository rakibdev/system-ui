add_requires("system-ui", {system = true})

target("demo")
  set_kind("shared")
  add_files("demo.cpp")
  add_packages("system-ui")
  after_build(function (target)
    os.cp(target:targetfile(), "~/.config/system-ui/extensions")
  end)