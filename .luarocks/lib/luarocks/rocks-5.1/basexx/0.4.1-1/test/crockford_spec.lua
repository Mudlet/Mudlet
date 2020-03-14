basexx = require( "basexx" )

describe( "should handle base32(crockford) strings", function()

   it( "should fulfill crockford-py test case", function()
      -- https://github.com/ingydotnet/crockford-py/blob/master/tests/test_functions.py
      assert.is.same( "CSQPY", basexx.to_crockford( "foo" ) )
      assert.is.same( "foo", basexx.from_crockford( "CSQPY" ) )
   end)

   it( "should really work ;-)", function()
      -- https://github.com/aiq/basexx/issues/3
      assert.is.same( "91jprv3f41bpywkccg", string.lower( basexx.to_crockford( "Hello World" ) ) )
      assert.is.same( "AXQQEB10D5T20WK5C5P6RY90EXQQ4TVK44", basexx.to_crockford( "Wow, it really works!" ) )
      assert.is.same( "Wow, it really works!", basexx.from_crockford( "axqqeb10d5t20wk5c5p6ry90exqq4tvk44" ) )
   end)

   it( "should allow to ignore characters in a crockford string", function()
      assert.is.same( "foo", basexx.from_crockford( "CSQPY\n", "\n" ) )
   end)

   it( "should handle wrong characters without a crash", function()
      local res, err = basexx.from_crockford( "CSQ%PY" )
      assert.is.falsy( res )
      assert.is.same( "unexpected character at position 4: '%'", err )
   end)

end)
