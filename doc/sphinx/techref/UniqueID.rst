UniqueID and XUID
=================

.. epigraph::
   | C makes it easy to shoot yourself in the foot;
   | C++ makes it harder, but when you do,
   | it blows away your whole leg."

   -- Bjarne Stroustrup

| From: Thomas Phinney of Adobe
| Subject: [OpenType] Why you don't need UniqueIDs
| Date:   Fri, 04 Mar 2005 10:04:41 -0800
|
| Neither Type 1 nor OpenType CFF fonts require PostScript UniqueIDs.
|
| Back in the mid-80s, when printing might be done on a 57K serial connection
| and printers might have 8 MHz processors, the caching of device-generated
| bitmaps enabled by UniqueIDs made a noticeable speed difference. With today's
| connection bandwidths and printers, the printing speed difference was
| insignificant in our tests (conducted around 2002), and certainly not enough
| to outweigh the risks of collisions between UniqueIDs for different fonts and
| the trouble of tracking the ID numbers.
|
| For these reasons, Adobe stopped using UniqueIDs (and XUIDs) in our own
| OpenType CFF fonts. If we still made Type 1 fonts, we wouldn't use UniqueIDs
| for them, either.
|
| You may of course continue to use UniqueIDs, it's just that they are not
| necessary.
|
| Regards,
|
| T
|
| Thomas W. Phinney
| Program Manager
| Fonts & Core Technologies
| Adobe Systems

