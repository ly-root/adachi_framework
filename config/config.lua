local configs = {}
for _, module in ipairs(list_dirs(project_dir)) do
    local config_file = project_dir .. "/" .. module .. "/config/config.lua"
    local f = io.open(config_file, "r")
    if f then
        f:close()
        local config = dofile(config_file)
        configs[module] = config
    end
end
configs["foxglove"] = require("foxglove")
return configs
