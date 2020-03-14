basexx = require( "basexx" )

describe( "should handle hex strings", function()

   it( "should convert data to a hex string", function()
      assert.is.same( "48656C6C6F20776F726C6421",
                      basexx.to_hex( "Hello world!" ) )
   end)

   it( "should read data from a upper and lower hex string", function()
      assert.is.same( "Hello world!",
                      basexx.from_hex( "48656C6C6F20776F726C6421" ) )
      assert.is.same( "Hello world!",
                      basexx.from_hex( "48656c6c6f20776f726c6421" ) )
   end)

   it( "should allow to ignore characters in a hex string", function()
      assert.is.same( "Hello world!",
                      basexx.from_hex( "4865-6c6c 6f20-776f 726c-6421", " -" ) )
   end)

   it( "should handle wrong characters without a crash", function()
      local res, err = basexx.from_hex( "4865-6c6c" )
      assert.is.falsy( res )
      assert.is.same( "unexpected character at position 5: '-'", err )
   end)

end)
