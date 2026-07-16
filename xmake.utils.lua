function setup()
	set_languages("c++26")
	set_toolchains("clang")
	set_defaultmode("debug")
	add_rules("mode.debug", "mode.release")
	add_cxxflags("-Wno-absolute-value")
	add_rules("c++.build.modules")

	if is_mode("debug") then
		add_defines("DEV")
	end
end

function build_in_root()
	set_targetdir(os.projectdir() .. "/build")
end

function sdk(path)
	return os.projectdir() .. "/src/" .. path
end

function set_extension(extName)
	set_kind("shared")
	set_targetdir(os.projectdir() .. "/build")

	-- Fixes linking relocation for .so extensions.
	add_cxxflags("-fPIC")

	-- Remove "lib" prefix
	set_prefixname("")

	add_deps("system-ui")
	set_policy("build.c++.modules.reuse", true)

	-- Must match system-ui's packages exactly, or module reuse silently falls back to
	-- recompiling every BMI. add_deps({public=true}) only propagates includedirs/links,
	-- not each package's own cxxflags (e.g. gtk4 adds -mfpmath=sse/-msse/-msse2/-pthread,
	add_packages("gtk4", "gtk4-layer-shell", "libpipewire-0.3", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0")

	local envExtDir = os.getenv("EXT_DIR")
	add_defines('EXT_DIR="' .. (envExtDir or os.scriptdir()) .. '"')
end
