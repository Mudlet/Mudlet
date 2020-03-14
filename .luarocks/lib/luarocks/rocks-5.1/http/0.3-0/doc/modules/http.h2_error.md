## http.h2_error

A type of error object that encapsulates HTTP 2 error information.
An `http.h2_error` object has fields:

  - `name`: The error name: a short identifier for this error
  - `code`: The error code
  - `description`: The description of the error code
  - `message`: An error message
  - `traceback`: A traceback taken at the point the error was thrown
  - `stream_error`: A boolean that indicates if this is a stream level or protocol level error

### `errors` <!-- --> {#http.h2_error.errors}

A table containing errors [as defined by the HTTP 2 specification](https://http2.github.io/http2-spec/#iana-errors).
It can be indexed by error name (e.g. `errors.PROTOCOL_ERROR`) or numeric code (e.g. `errors[0x1]`).


### `is(ob)` <!-- --> {#http.h2_error.is}

Returns a boolean indicating if the object `ob` is an `http.h2_error` object


### `h2_error:new(ob)` <!-- --> {#http.h2_error:new}

Creates a new error object from the passed table.
The table should have the form of an error object i.e. with fields `name`, `code`, `message`, `traceback`, etc.

Fields `name`, `code` and `description` are inherited from the parent `h2_error` object if not specified.

`stream_error` defaults to `false`.


### `h2_error:new_traceback(message, stream_error, lvl)` <!-- --> {#http.h2_error:new_traceback}

Creates a new error object, recording a traceback from the current thread.


### `h2_error:error(message, stream_error, lvl)` <!-- --> {#http.h2_error:error}

Creates and throws a new error.


### `h2_error:assert(cond, ...)` <!-- --> {#http.h2_error:assert}

If `cond` is truthy, returns `cond, ...`

If `cond` is falsy (i.e. `false` or `nil`), throws an error with the first element of `...` as the `message`.
