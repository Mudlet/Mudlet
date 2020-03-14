basexx = require( "basexx" )

describe( "should handle base32(rfc3548) strings", function()
   
   it( "should convert chunky bacon", function()
      -- https://github.com/stesla/base32
      assert.is.same( "MNUHK3TLPEQGEYLDN5XCC===",
                      basexx.to_base32( "chunky bacon!" ) )
      assert.is.same( "chunky bacon!",
                      basexx.from_base32( "MNUHK3TLPEQGEYLDN5XCC===" ) )
   end)

   it( "should allow to ignore characters in a base32 string", function()
      assert.is.same( "chunky bacon!",
                      basexx.from_base32( "MNUHK3TLPEQGEYLDN5XCC===" ) )
   end)

   it ( "should handle wrong characters without a crash", function()
      local res, err = basexx.from_base32( "MS$DF" )
      assert.is.falsy( res )
      assert.is.same( "unexpected character at position 3: '$'", err )
   end)

end)
