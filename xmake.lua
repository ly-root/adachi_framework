add_repositories("third_party third_party")
set_project("project")
set_version("1.0.0")
set_languages("c++23")
add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")
set_targetdir("build")

if is_mode("asan") then
    add_cflags("-fsanitize=address", "-fsanitize=leak", "-fsanitize=undefined",
              "-fno-omit-frame-pointer", "-fsanitize-address-use-after-scope")
    add_cxxflags("-fsanitize=address", "-fsanitize=leak", "-fsanitize=undefined",
                 "-fno-omit-frame-pointer", "-fsanitize-address-use-after-scope")
    add_ldflags("-fsanitize=address", "-fsanitize=leak", "-fsanitize=undefined")
    add_defines("ASAN_ENABLED")
end

rule("auto_object")
on_load(function(target)
    if target:name() ~= "app" then
        target:set("kind", "object")
    else
        target:set("kind", "binary")
    end
end)
rule_end()
add_rules("auto_object")

local node_dirs = {}
for _, dir in ipairs(os.dirs("*")) do
    local name = path.filename(dir)
    if name ~= "framework"
        and name ~= "app"
        and os.isfile(path.join(dir, "xmake.lua")) then
        table.insert(node_dirs, dir)
    end
end

local node_targets = {}
for _, dir in ipairs(node_dirs) do
    if os.isfile(path.join(dir, "xmake.lua")) then
        local name = path.filename(dir)
        table.insert(node_targets, name)
        option(name)
        set_default(true)
        set_showmenu(true)
    end
end

local disabled_nodes = {}
enabled_nodes = {}
for _, name in ipairs(node_targets) do
    if has_config(name) then
        node_name = name
        includes(name)
        table.insert(enabled_nodes, name)
    else
        table.insert(disabled_nodes, name)
    end
end

includes("framework")
includes("app")

target("summary")
set_kind("phony")
add_deps("app")
after_build(function(target)
    if #disabled_nodes > 0 then
        for _, name in ipairs(disabled_nodes) do
            cprint(
                "${bright yellow}warning:${clear} %s disabled",
                name
            )
        end
    end
end)
