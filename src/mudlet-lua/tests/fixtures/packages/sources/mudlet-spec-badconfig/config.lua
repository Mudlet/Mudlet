mpackage = [[mudlet-spec-badconfig-renamed]]
author = [[Mudlet test suite]]
title = [[Fixture whose config.lua will not run, for Package_spec.lua]]
version = [[3.3]]
-- os is not in the sandbox config.lua is read in, so this line raises a runtime
-- error and the whole manifest above it is thrown away with it
stamp = os.date("%Y")
