basexx = require( "basexx" )

describe( "should handle ZeroMQ base85 strings", function()
   
   it( "should fulfill spec test case", function()
      -- http://rfc.zeromq.org/spec:32
      local data = string.char( 0x86, 0x4f, 0xd2, 0x6f, 0xb5, 0x59, 0xf7, 0x5b )
      local z85 = "HelloWorld"
      assert.is.same( z85, basexx.to_z85( data ) )
      assert.is.same( data, basexx.from_z85( z85 ) )
   end)

   it( "should encode a numeric string correctly", function()
      -- https://github.com/msealand/z85.node/blob/master/test/encode.test.js
      assert.is.same( "f!$Kw", basexx.to_z85( "1234" ) )
      assert.is.same( "1234", basexx.from_z85( "f!$Kw" ) )
   end)

   it( "should allow to ignore characters in a base85 string", function()
      assert.is.same( "1234", basexx.from_z85( "'f!$Kw'\n", "'\n" ) )
   end)

   it( "should handle wrong input lenght without a crash", function()
      local res, err = basexx.from_z85( "abcdefghi" )
      assert.is.falsy( res )
      assert.is.same( err, "invalid length: 9 - must be a multiple of 5" )

      res, err = basexx.to_z85( "abcdefghi" )
      assert.is.falsy( res )
      assert.is.same( err, "invalid length: 9 - must be a multiple of 4" )
   end)

   it( "should handle wrong characters without a crash", function()
      local c = string.char( 140 )
      local res, err = basexx.from_z85( "f"..c.."$Kw" )
      assert.is.falsy( res )
      local msg = string.format( "unexpected character at position 2: '%s'", c )
      assert.is.same( err, msg )
   end)

end)
