mpackage = [[mudlet-spec-silentconfig-renamed]]
author = [[Mudlet test suite]]
title = [[Fixture whose config.lua raises an error with no message, for Package_spec.lua]]
version = [[3.4]]
-- error() with nothing to say raises nil rather than a string, so there is no
-- message for the reader to report
error()
