/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
   This file implements utility routines for the rdf data model.
   For more information on this file, contact rjc or guha
   For more information on RDF, look at the RDF section of www.mozilla.org
*/

#define kLeft1BitMask  0x80
#define kLeft2BitsMask 0xC0
#define kLeft3BitsMask 0xE0
#define kLeft4BitsMask 0xF0
#define kLeft5BitsMask 0xF8
#define kLeft6BitsMask 0xFC
#define kLeft7BitsMask 0xFE

#define k2BytesLeadByte kLeft2BitsMask
#define k3BytesLeadByte kLeft3BitsMask
#define k4BytesLeadByte kLeft4BitsMask
#define k5BytesLeadByte kLeft5BitsMask
#define k6BytesLeadByte kLeft6BitsMask
#define kTrialByte      kLeft1BitMask

#define UTF8_1Byte(c) ( 0 == ((c) & kLeft1BitMask))
#define UTF8_2Bytes(c) ( k2BytesLeadByte == ((c) & kLeft3BitsMask))
#define UTF8_3Bytes(c) ( k3BytesLeadByte == ((c) & kLeft4BitsMask))
#define UTF8_4Bytes(c) ( k4BytesLeadByte == ((c) & kLeft5BitsMask))
#define UTF8_5Bytes(c) ( k5BytesLeadByte == ((c) & kLeft6BitsMask))
#define UTF8_6Bytes(c) ( k6BytesLeadByte == ((c) & kLeft7BitsMask))
#define UTF8_ValidTrialByte(c) ( kTrialByte == ((c) & kLeft2BitsMask))

int IsUTF8Text(const unsigned char* utf8, int len)
{
   int i;
   int j;
   int clen;
   for(i =0; i < len; i += clen)
   {
      if(UTF8_1Byte(utf8[i]))
      {
        clen = 1;
      } else if(UTF8_2Bytes(utf8[i])) {
        clen = 2;
    /* No enough trail bytes */
        if( (i + clen) > len)
      return 0;
    /* 0000 0000 - 0000 007F : should encode in less bytes */
        if(0 ==  (utf8[i] & 0x1E ))
          return 0;
      } else if(UTF8_3Bytes(utf8[i])) {
        clen = 3;
    /* No enough trail bytes */
        if( (i + clen) > len)
      return 0;
    /* a single Surrogate should not show in 3 bytes UTF8, instead, the pair should be intepreted
       as one single UCS4 char and encoded UTF8 in 4 bytes */
        if((0xED == utf8[i] ) && (0xA0 ==  (utf8[i+1] & 0xA0 ) ))
          return 0;
    /* 0000 0000 - 0000 07FF : should encode in less bytes */
        if((0 ==  (utf8[i] & 0x0F )) && (0 ==  (utf8[i+1] & 0x20 ) ))
          return 0;
      } else if(UTF8_4Bytes(utf8[i])) {
        clen = 4;
    /* No enough trail bytes */
        if( (i + clen) > len)
      return 0;
    /* 0000 0000 - 0000 FFFF : should encode in less bytes */
        if((0 ==  (utf8[i] & 0x07 )) && (0 ==  (utf8[i+1] & 0x30 )) )
          return 0;
      } else if(UTF8_5Bytes(utf8[i])) {
        clen = 5;
    /* No enough trail bytes */
        if( (i + clen) > len)
      return 0;
    /* 0000 0000 - 001F FFFF : should encode in less bytes */
        if((0 ==  (utf8[i] & 0x03 )) && (0 ==  (utf8[i+1] & 0x38 )) )
          return 0;
      } else if(UTF8_6Bytes(utf8[i])) {
        clen = 6;
    /* No enough trail bytes */
        if( (i + clen) > len)
      return 0;
    /* 0000 0000 - 03FF FFFF : should encode in less bytes */
        if((0 ==  (utf8[i] & 0x01 )) && (0 ==  (utf8[i+1] & 0x3E )) )
          return 0;
      } else {
        return 0;
      }
      for(j = 1; j<clen ;j++)
      {
    if(! UTF8_ValidTrialByte(utf8[i+j])) /* Trail bytes invalid */
      return 0;
      }
   }
   return 1;
}
