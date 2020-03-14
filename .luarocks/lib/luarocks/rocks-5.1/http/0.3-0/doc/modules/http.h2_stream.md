## http.h2_stream

An h2_stream represents an HTTP 2 stream. The module follows the [*stream*](#stream) interface as well as HTTP 2 specific functions.

### `h2_stream.connection` <!-- --> {#http.h2_stream.connection}

See [`stream.connection`](#stream.connection)


### `h2_stream:checktls()` <!-- --> {#http.h2_stream:checktls}

See [`stream:checktls()`](#stream:checktls)


### `h2_stream:localname()` <!-- --> {#http.h2_stream:localname}

See [`stream:localname()`](#stream:localname)


### `h2_stream:peername()` <!-- --> {#http.h2_stream:peername}

See [`stream:peername()`](#stream:peername)


### `h2_stream:get_headers(timeout)` <!-- --> {#http.h2_stream:get_headers}

See [`stream:get_headers(timeout)`](#stream:get_headers)


### `h2_stream:write_headers(headers, end_stream, timeout)` <!-- --> {#http.h2_stream:write_headers}

See [`stream:write_headers(headers, end_stream, timeout)`](#stream:write_headers)


### `h2_stream:write_continue(timeout)` <!-- --> {#http.h2_stream:write_continue}

See [`stream:write_continue(timeout)`](#stream:write_continue)


### `h2_stream:get_next_chunk(timeout)` <!-- --> {#http.h2_stream:get_next_chunk}

See [`stream:get_next_chunk(timeout)`](#stream:get_next_chunk)


### `h2_stream:each_chunk()` <!-- --> {#http.h2_stream:each_chunk}

See [`stream:each_chunk()`](#stream:each_chunk)


### `h2_stream:get_body_as_string(timeout)` <!-- --> {#http.h2_stream:get_body_as_string}

See [`stream:get_body_as_string(timeout)`](#stream:get_body_as_string)


### `h2_stream:get_body_chars(n, timeout)` <!-- --> {#http.h2_stream:get_body_chars}

See [`stream:get_body_chars(n, timeout)`](#stream:get_body_chars)


### `h2_stream:get_body_until(pattern, plain, include_pattern, timeout)` <!-- --> {#http.h2_stream:get_body_until}

See [`stream:get_body_until(pattern, plain, include_pattern, timeout)`](#stream:get_body_until)


### `h2_stream:save_body_to_file(file, timeout)` <!-- --> {#http.h2_stream:save_body_to_file}

See [`stream:save_body_to_file(file, timeout)`](#stream:save_body_to_file)


### `h2_stream:get_body_as_file(timeout)` <!-- --> {#http.h2_stream:get_body_as_file}

See [`stream:get_body_as_file(timeout)`](#stream:get_body_as_file)


### `h2_stream:unget(str)` <!-- --> {#http.h2_stream:unget}

See [`stream:unget(str)`](#stream:unget)


### `h2_stream:write_chunk(chunk, end_stream, timeout)` <!-- --> {#http.h2_stream:write_chunk}

See [`stream:write_chunk(chunk, end_stream, timeout)`](#stream:write_chunk)


### `h2_stream:write_body_from_string(str, timeout)` <!-- --> {#http.h2_stream:write_body_from_string}

See [`stream:write_body_from_string(str, timeout)`](#stream:write_body_from_string)


### `h2_stream:write_body_from_file(options|file, timeout)` <!-- --> {#http.h2_stream:write_body_from_file}

See [`stream:write_body_from_file(options|file, timeout)`](#stream:write_body_from_file)


### `h2_stream:shutdown()` <!-- --> {#http.h2_stream:shutdown}

See [`stream:shutdown()`](#stream:shutdown)


### `h2_stream:pick_id(id)` <!-- --> {#http.h2_stream:pick_id}


### `h2_stream:set_state(new)` <!-- --> {#http.h2_stream:set_state}


### `h2_stream:reprioritise(child, exclusive)` <!-- --> {#http.h2_stream:reprioritise}


### `h2_stream:write_http2_frame(typ, flags, payload, timeout, flush)` <!-- --> {#http.h2_stream:write_http2_frame}

Writes a frame with `h2_stream`'s stream id.

See [`h2_connection:write_http2_frame(typ, flags, streamid, payload, timeout, flush)`](#http.h2_connection:write_http2_frame)


### `h2_stream:write_data_frame(payload, end_stream, padded, timeout, flush)` <!-- --> {#http.h2_stream:write_data_frame}


### `h2_stream:write_headers_frame(payload, end_stream, end_headers, padded, exclusive, stream_dep, weight, timeout, flush)` <!-- --> {#http.h2_stream:write_headers_frame}


### `h2_stream:write_priority_frame(exclusive, stream_dep, weight, timeout, flush)` <!-- --> {#http.h2_stream:write_priority_frame}


### `h2_stream:write_rst_stream_frame(err_code, timeout, flush)` <!-- --> {#http.h2_stream:write_rst_stream}


### `h2_stream:rst_stream(err, timeout)` <!-- --> {#http.h2_stream:rst_stream}


### `h2_stream:write_settings_frame(ACK, settings, timeout, flush)` <!-- --> {#http.h2_stream:write_settings_frame}


### `h2_stream:write_push_promise_frame(promised_stream_id, payload, end_headers, padded, timeout, flush)` <!-- --> {#http.h2_stream:write_push_promise_frame}


### `h2_stream:push_promise(headers, timeout)` <!-- --> {#http.h2_stream:push_promise}

Pushes a new promise to the client.

Returns the new stream as a [h2_stream](#http.h2_stream).


### `h2_stream:write_ping_frame(ACK, payload, timeout, flush)` <!-- --> {#http.h2_stream:write_ping_frame}


### `h2_stream:write_goaway_frame(last_streamid, err_code, debug_msg, timeout, flush)` <!-- --> {#http.h2_stream:write_goaway_frame}


### `h2_stream:write_window_update_frame(inc, timeout, flush)` <!-- --> {#http.h2_stream:write_window_update_frame}


### `h2_stream:write_window_update(inc, timeout)` <!-- --> {#http.h2_stream:write_window_update}


### `h2_stream:write_continuation_frame(payload, end_headers, timeout, flush)` <!-- --> {#http.h2_stream:write_continuation_frame}
