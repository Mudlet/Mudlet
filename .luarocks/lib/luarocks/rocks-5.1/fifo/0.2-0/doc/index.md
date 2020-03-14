---
title: fifo.lua
author:
  - daurnimator <quae@daurnimator.com>
section: 3
---

# DESCRIPTION

A lua library/'class' that implements a FIFO.
Objects in the fifo can be of any type, including `nil`.


# USAGE

The library returns the constructor `fifo.new`:

```lua
new_fifo = require "fifo"
```

## `myfifo:fifo = new_fifo(...)`

Create a new fifo by calling the constructor;
it optionally takes the initial state.

```lua
myfifo = new_fifo("foo", "bar")
```

## `myfifo = myfifo:setempty(f:function)`

The behaviour when trying to `:pop()` or `:remove()` too many items from an empty list is configurable.
Returns the fifo itself

By default an error will be thrown.
You can set a custom behaviour by providing a function to `:setempty()`.
The return values of your function will be returned by `:pop()` or `:remove()`

```lua
myfifo:setempty(function(myfifo) return nil end)
```

This method returns self, which makes it easy to use at construction time:
e.g. to create a new fifo where `:pop()` returns `nil` when empty:

```lua
myfifo = new_fifo():setempty(function() return nil end)
```


## `fifo:push(object:*)`

Use the `:push()` method to append an object to the fifo

```lua
myfifo:push({"an object"})
```


## `object:*, exists:bool = fifo:peek(n:number|none)`

Allows you to inspect a fifo without removing items from it.
Returns the item at the given index (or `nil`) and whether it existed (as `nil` is a valid value).
By default uses the next item from the fifo.

```lua
exists, myobject = myfifo:peek()
```


## `object:* = fifo:pop()`

Returns the next item from the fifo, removing it.

```lua
myobject = myfifo:pop()
```


## `fifo:insert(index:number, object:*)`

This can be used to insert an item into the middle of a fifo.
The index is from the output of the fifo
where `1` would be the next item popped from the fifo,
and `myfifo:length() + 1` would be equivalent to `:push()`
The efficiency of this operation is proportional to the distance from either end of the fifo.

```lua
myobject = myfifo:insert(1, {"some object"})
```


## `object:* = fifo:remove(index:number)`

This can be used to remove an item from the middle of a fifo.
The index is from the output of the fifo
where `1` would be the next item popped from the fifo,
and `myfifo:length()` would be the input (i.e equivalent to `:push()`)
The efficiency of this operation is proportional to the distance from either end of the fifo.

The object removed is returned.

```lua
myobject = myfifo:remove(2)
```


## `length:number = fifo:length()` and `length:number = #fifo` operator

Returns the current number of items in the fifo.
Available as `:length()` as the `__len` metamethod doesn't work for tables in lua versions 5.1 and earlier.
