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
                  "bGVhc3VyZS4"

   it( "should convert data to a base64 string", function()
      -- http://en.wikipedia.org/wiki/Base64#URL_applications
      assert.is.same( 'TWFu', basexx.to_url64( 'Man') )
      assert.is.same( 'Man', basexx.from_url64( 'TWFu') )

      assert.is.same( 'bGVhc3VyZS4', basexx.to_url64( 'leasure.') )
      assert.is.same( 'leasure.', basexx.from_url64( 'bGVhc3VyZS4') )

      assert.is.same( 'cGxlYXN1cmUu', basexx.to_url64( 'pleasure.') )
      assert.is.same( 'pleasure.', basexx.from_url64( 'cGxlYXN1cmUu') )

      assert.is.same( 'ZWFzdXJlLg', basexx.to_url64( 'easure.') )
      assert.is.same( 'easure.', basexx.from_url64( 'ZWFzdXJlLg') )

      assert.is.same( 'c3VyZS4', basexx.to_url64( 'sure.') )
      assert.is.same( 'sure.', basexx.from_url64( 'c3VyZS4') )

      assert.is.same( long64, basexx.to_url64( longtxt ) )
      assert.is.same( longtxt, basexx.from_url64( long64 ) )
   end)

   local msgtxt = '{"msg_en":"Hello","msg_jp":"こんにちは","msg_cn":"你好"'..
                  ',"msg_kr":"안녕하세요","msg_ru":"Здравствуйте!"'..
                  ',"msg_de":"Grüß Gott"}'

   local msg64 = 'eyJtc2dfZW4iOiJIZWxsbyIsIm1zZ19qcCI6IuOBk-OCk-OBq-OBoeOBryI'..
                 'sIm1zZ19jbiI6IuS9oOWlvSIsIm1zZ19rciI6IuyViOuFle2VmOyEuOyalC'..
                 'IsIm1zZ19ydSI6ItCX0LTRgNCw0LLRgdGC0LLRg9C50YLQtSEiLCJtc2dfZ'..
                 'GUiOiJHcsO8w58gR290dCJ9'

   it( "should work with the msg example", function()
      assert.is.same( msg64, basexx.to_url64( msgtxt ) )
      assert.is.same( msgtxt, basexx.from_url64( msg64 ) )
   end)

   it( "should allow to ignore characters in a url64 string", function()
      assert.is.same( "Man", basexx.from_url64( "TW-Fu\n", "-\n" ) )
   end)

end)
