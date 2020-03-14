## http.hsts

Data structures useful for HSTS (HTTP Strict Transport Security)

### `new_store()` <!-- --> {#http.hsts.new_store}

Creates and returns a new HSTS store.


### `hsts_store.max_items` <!-- --> {#http.hsts.max_items}

The maximum number of items allowed in the store.
Decreasing this value will only prevent new items from being added, it will not remove old items.

Defaults to infinity (any number of items is allowed).


### `hsts_store:clone()` <!-- --> {#http.hsts:clone}

Creates and returns a copy of a store.


### `hsts_store:store(host, directives)` <!-- --> {#http.hsts:store}

Add new directives to the store about the given `host`. `directives` should be a table of directives, which *must* include the key `"max-age"`.

Returns a boolean indicating if the item was accepted.


### `hsts_store:remove(host)` <!-- --> {#http.hsts:remove}

Removes the entry for `host` from the store (if it exists).


### `hsts_store:check(host)` <!-- --> {#http.hsts:check}

Returns a boolean indicating if the given `host` is a known HSTS host.


### `hsts_store:clean_due()` <!-- --> {#http.hsts:clean_due}

Returns the number of seconds until the next item in the store expires.


### `hsts_store:clean()` <!-- --> {#http.hsts:clean}

Removes expired entries from the store.
