Unicode Corporate Characters used by FontForge
==============================================

Unicode has a region (0xe000-0xf8ff) in which font vendors may assign their own
characters. Adobe and Apple have each done so and FontForge recognizes the most
common of these (things like the Small Caps characters used in Adobe's Expert
Character Set, or the Apple Logo used in nearly all Apple fonts). The
allocations made by these two companies are described in:

* `Apple's corporate use extensions <http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT>`_
  (0xF850-0xF8FE)
* `Adobe's corporate use extensions <http://partners.adobe.com/asn/tech/type/corporateuse.txt>`_
  (0xF634-0F7FF) (also includes some of Apple's codes above)

I have been asked to add a few similar characters to FontForge's repertory and
so have started using my own block of characters 0xf500-0xf580.

.. highlight:: none


Greek Small Caps
----------------

::

   f500;Alphasmall;GREEK SMALL CAPITAL LETTER ALPHA;<sc>;0391
   f501;Betasmall;GREEK SMALL CAPITAL LETTER BETA;<sc>;0392
   f502;Gammasmall;GREEK SMALL CAPITAL LETTER GAMMA;<sc>;0393
   f503;Deltasmall;GREEK SMALL CAPITAL LETTER DELTA;<sc>;0394
   f504;Epsilonsmall;GREEK SMALL CAPITAL LETTER EPSILON;<sc>;0395
   f505;Zetasmall;GREEK SMALL CAPITAL LETTER ZETA;<sc>;0396
   f506;Etasmall;GREEK SMALL CAPITAL LETTER ETA;<sc>;0397
   f507;Thetasmall;GREEK SMALL CAPITAL LETTER THETA;<sc>;0398
   f508;Iotasmall;GREEK SMALL CAPITAL LETTER IOTA;<sc>;0399
   f509;Kappasmall;GREEK SMALL CAPITAL LETTER KAPPA;<sc>;039a
   f50a;Lambdasmall;GREEK SMALL CAPITAL LETTER LAMBDA;<sc>;039b
   f50b;Musmall;GREEK SMALL CAPITAL LETTER MU;<sc>;039c
   f50c;Nusmall;GREEK SMALL CAPITAL LETTER NU;<sc>;039d
   f50d;Xismall;GREEK SMALL CAPITAL LETTER XI;<sc>;039e
   f50e;Omicronsmall;GREEK SMALL CAPITAL LETTER OMICRON;<sc>;039f
   f50f;Pismall;GREEK SMALL CAPITAL LETTER ALPHA;<sc>;03a0
   f510;Rhosmall;GREEK SMALL CAPITAL LETTER RHO;<sc>;03a1
   f511;;
   f512;Sigmasmall;GREEK SMALL CAPITAL LETTER SIGMA;<sc>;03a3
   f513;Tausmall;GREEK SMALL CAPITAL LETTER TAU;<sc>;03a4
   f514;Upsilonsmall;GREEK SMALL CAPITAL LETTER UPSILON;<sc>;03a5
   f515;Phismall;GREEK SMALL CAPITAL LETTER PHI;<sc>;03a6
   f516;Chismall;GREEK SMALL CAPITAL LETTER CHI;<sc>;03a7
   f517;Psismall;GREEK SMALL CAPITAL LETTER PSI;<sc>;03a8
   f518;Omegasmall;GREEK SMALL CAPITAL LETTER OMEGA;<sc>;03a9
   f519;Iotadieresissmall;GREEK SMALL CAPITAL LETTER IOTA WITH DIAERESIS;<sc>;03aa
   f51a;Upsilondieresissmall;GREEK SMALL CAPITAL LETTER UPSILON WITH DIAERESIS;<sc>;03ab


Archaic and Rare Latin Ligatures
--------------------------------

::

   f520;c_t;LATIN SMALL LIGATURE CT;;;0063 0074
   f521;longs_i;LATIN SMALL LIGATURE LONGS I;;;017f 0069
   f522;longs_l;LATIN SMALL LIGATURE LONGS L;;;017f 006c
   f523;longs_longs;LATIN SMALL LIGATURE LONGS LONGS;;;017f 017f
   f524;longs_longs_i;LATIN SMALL LIGATURE LONGS LONGS I;;;017f 0x17f 0069
   f525;longs_longs_l;LATIN SMALL LIGATURE LONGS LONGS L;;;017f 0x17f 006c
