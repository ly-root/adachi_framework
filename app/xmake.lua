target("app")
add_files("src/*.cpp")
add_deps("framework")
add_includedirs("include")
add_defines("PROJECT_DIR_PATH=\"" .. path.absolute(os.projectdir()) .. "\"")
after_load(function(target)
    import("core.project.project")
    target:set("filename", project.name())
end)

for _, name in ipairs(enabled_nodes) do
    node_name = name
    includes("../" .. name)
    add_deps(name)
end
