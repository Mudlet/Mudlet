basexx = require( "basexx" )

describe( "should handle base64 strings", function()

   local longtxt = "Man is distinguished, not only by his reason, but by "..
                   "this singular passion from other animals, which is a "..
                   "lust of the mind, that by a perseverance of delight in "..
                   "the continued and indefatigable generation of knowledge, "..
                   "exceeds the short vehemence of any carnal pleasure."

   local long64 = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb2"..
                  "4sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBh"..
                  "bmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYn"..
                  "kgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVk"..
                  "IGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLC"..
                  "BleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBw"..
                  "bGVhc3VyZS4="

   it( "should work with wikipedia examples", function()
      -- http://en.wikipedia.org/wiki/Base64
      assert.is.same( 'TWFu', basexx.to_base64( 'Man') )
      assert.is.same( 'Man', basexx.from_base64( 'TWFu') )

      assert.is.same( 'bGVhc3VyZS4=', basexx.to_base64( 'leasure.') )
      assert.is.same( 'leasure.', basexx.from_base64( 'bGVhc3VyZS4=') )

      assert.is.same( 'cGxlYXN1cmUu', basexx.to_base64( 'pleasure.') )
      assert.is.same( 'pleasure.', basexx.from_base64( 'cGxlYXN1cmUu') )
      
      assert.is.same( 'ZWFzdXJlLg==', basexx.to_base64( 'easure.') )
      assert.is.same( 'easure.', basexx.from_base64( 'ZWFzdXJlLg==') )
      
      assert.is.same( 'c3VyZS4=', basexx.to_base64( 'sure.') )
      assert.is.same( 'sure.', basexx.from_base64( 'c3VyZS4=') )

      assert.is.same( long64, basexx.to_base64( longtxt ) )
      assert.is.same( longtxt, basexx.from_base64( long64) )
   end)

   it( "should handle padding correct", function()
      -- http://en.wikipedia.org/wiki/Base64#Padding
      assert.is.same( "YW55IGNhcm5hbCBwbGVhc3VyZS4=",
                      basexx.to_base64( "any carnal pleasure." ) )
      assert.is.same( "YW55IGNhcm5hbCBwbGVhc3VyZQ==",
                      basexx.to_base64( "any carnal pleasure" ) )
      assert.is.same( "YW55IGNhcm5hbCBwbGVhc3Vy",
                      basexx.to_base64( "any carnal pleasur" ) )
      assert.is.same( "YW55IGNhcm5hbCBwbGVhc3U=",
                      basexx.to_base64( "any carnal pleasu" ) )
      assert.is.same( "YW55IGNhcm5hbCBwbGVhcw==",
                      basexx.to_base64( "any carnal pleas" ) )

      assert.is.same( "any carnal pleas",
                      basexx.from_base64( "YW55IGNhcm5hbCBwbGVhcw==" ) )
      assert.is.same( "any carnal pleasu",
                      basexx.from_base64( "YW55IGNhcm5hbCBwbGVhc3U=" ) )
      assert.is.same( "any carnal pleasur",
                      basexx.from_base64( "YW55IGNhcm5hbCBwbGVhc3Vy" ) )
   end)

   it( "should allow to ignore characters in a base64 string", function()
      assert.is.same( "Man", basexx.from_base64( "TW-Fu", "-" ) )
   end)

   it( "should handle wrong characters without a crash", function()
      local res, err = basexx.from_base64( "TW`Fu" )
      assert.is.falsy( res )
      assert.is.same( "unexpected character at position 3: '`'", err )
   end)

end)
