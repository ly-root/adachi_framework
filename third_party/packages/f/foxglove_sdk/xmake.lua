package("foxglove_sdk")
    set_kind("library")
    set_homepage("https://foxglove.dev/")
    set_description("Foxglove SDK")

    add_includedirs("include")
    add_links("foxglove")
    add_linkdirs("lib")

    on_install(function(package)
        local scriptdir = os.scriptdir()
        local builddir = package:builddir()
        local installdir = package:installdir()
        os.mkdir(builddir)

        local objects = {}
        for _, srcfile in ipairs(os.files(path.join(scriptdir, "src", "*.cpp"))) do
            local obj = path.join(builddir, path.basename(srcfile) .. ".o")
            table.insert(objects, obj)
            os.runv("c++", {"-c", "-std=c++17", "-fPIC",
                            "-I", path.join(scriptdir, "include"),
                            "-o", obj, srcfile})
        end

        local combined = path.join(builddir, "libfoxglove.a")
        os.cp(path.join(scriptdir, "lib", "libfoxglove.a"), combined)
        os.runv("ar", table.join({"rcs", combined}, objects))

        os.cp(path.join(scriptdir, "include"), installdir)
        os.mkdir(path.join(installdir, "lib"))
        os.cp(combined, path.join(installdir, "lib", "libfoxglove.a"))
    end)
