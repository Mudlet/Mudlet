## http.hpack

### `new(SETTINGS_HEADER_TABLE_SIZE)` <!-- --> {#http.hpack.new}


### `hpack_context:append_data(val)` <!-- --> {#http.hpack:append_data}


### `hpack_context:render_data()` <!-- --> {#http.hpack:render_data}


### `hpack_context:clear_data()` <!-- --> {#http.hpack:clear_data}


### `hpack_context:evict_from_dynamic_table()` <!-- --> {#http.hpack:evict_from_dynamic_table}


### `hpack_context:dynamic_table_tostring()` <!-- --> {#http.hpack:dynamic_table_tostring}


### `hpack_context:set_max_dynamic_table_size(SETTINGS_HEADER_TABLE_SIZE)` <!-- --> {#http.hpack:set_max_dynamic_table_size}


### `hpack_context:encode_max_size(val)` <!-- --> {#http.hpack:encode_max_size}


### `hpack_context:resize_dynamic_table(new_size)` <!-- --> {#http.hpack:resize_dynamic_table}


### `hpack_context:add_to_dynamic_table(name, value, k)` <!-- --> {#http.hpack:add_to_dynamic_table}


### `hpack_context:dynamic_table_id_to_index(id)` <!-- --> {#http.hpack:dynamic_table_id_to_index}


### `hpack_context:lookup_pair_index(k)` <!-- --> {#http.hpack:lookup_pair_index}


### `hpack_context:lookup_name_index(name)` <!-- --> {#http.hpack:lookup_name_index}


### `hpack_context:lookup_index(index)` <!-- --> {#http.hpack:lookup_index}


### `hpack_context:add_header_indexed(name, value, huffman)` <!-- --> {#http.hpack:add_header_indexed}


### `hpack_context:add_header_never_indexed(name, value, huffman)` <!-- --> {#http.hpack:add_header_never_indexed}


### `hpack_context:encode_headers(headers)` <!-- --> {#http.hpack:encode_headers}


### `hpack_context:decode_headers(payload, header_list, pos)` <!-- --> {#http.hpack:decode_headers}
