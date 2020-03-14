## http.h1_reason_phrases

A table mapping from status codes (as strings) to reason phrases for HTTP 1. Any unknown status codes return `"Unassigned"`

### Example {#http.h1_reason_phrases-example}

```lua
local reason_phrases = require "http.h1_reason_phrases"
print(reason_phrases["200"]) --> "OK"
print(reason_phrases["342"]) --> "Unassigned"
```
