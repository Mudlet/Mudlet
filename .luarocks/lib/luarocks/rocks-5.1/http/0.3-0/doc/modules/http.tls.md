## http.tls

### `has_alpn` <!-- --> {#http.tls.has_alpn}

Boolean indicating if ALPN is available in the current environment.

It may be disabled if OpenSSL was compiled without ALPN support, or is an old version.


### `has_hostname_validation` <!-- --> {#http.tls.has_hostname_validation}

Boolean indicating if [hostname validation](https://wiki.openssl.org/index.php/Hostname_validation) is available in the current environment.

It may be disabled if OpenSSL is an old version.


### `modern_cipher_list` <!-- --> {#http.tls.modern_cipher_list}

The [Mozilla "Modern" cipher list](https://wiki.mozilla.org/Security/Server_Side_TLS#Modern_compatibility) as a colon separated list, ready to pass to OpenSSL


### `intermediate_cipher_list` <!-- --> {#http.tls.intermediate_cipher_list}

The [Mozilla "Intermediate" cipher list](https://wiki.mozilla.org/Security/Server_Side_TLS#Intermediate_compatibility_.28default.29) as a colon separated list, ready to pass to OpenSSL


### `old_cipher_list` <!-- --> {#http.tls.old_cipher_list}

The [Mozilla "Old" cipher list](https://wiki.mozilla.org/Security/Server_Side_TLS#Old_backward_compatibility) as a colon separated list, ready to pass to OpenSSL


### `banned_ciphers` <!-- --> {#http.tls.banned_ciphers}

A set (table with string keys and values of `true`) of the [ciphers banned in HTTP 2](https://http2.github.io/http2-spec/#BadCipherSuites) where the keys are OpenSSL cipher names.

Ciphers not known by OpenSSL are missing from the set.


### `new_client_context()` <!-- --> {#http.tls.new_client_context}

Create and return a new luaossl SSL context useful for HTTP client connections.


### `new_server_context()` <!-- --> {#http.tls.new_server_context}

Create and return a new luaossl SSL context useful for HTTP server connections.
