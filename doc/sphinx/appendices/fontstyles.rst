Font Styles in various languages
================================

..
   TODO: Figure out a better representation for this

.. container:: overflow-x-auto

   .. list-table::
      :header-rows: 1
      :stub-columns: 1
      :class: compact
   
      * - English
        - Regular
        - Bold
        - Demi-Bold
        - Light
        - Medium
        - Book
        - Black
        - Italic
        - Oblique
        - Condensed
        - Expanded
        - Outline
      * - :small:`simplified`
   
          Chinese
        - 正常
        - 粗体
        - 略粗
        - 细
        - 中等
        - 书体
        - 黑
        - 斜体
        -
        - 压缩
        - 加宽
        - 轮廓
      * - Croatian
        - Normalni
        - Debeli
        - Polu-Debeli
        - Svijetli
        - Srednji
        - Knjižni
        - Tamni
        - Kurziv
        - Ukošeni
        - Suženi
        - Prošireni
        - Konturni
      * - Danish
        - Normal
        - Fed
        - Halvfed
        - Fin
        - Medium
        -
        - Extra fed
        - Kursiv
        -
        - Smal
        - Bred
        - Kontur
      * - Dutch
        - Normaal
        - Vet
        - Dik
        - Licht
        - Gemiddeld
        - Boek
        - Extra vet
        - Cursief
        - Schuin
        - Smal
        - Breed
        - Uitlijning
      * - French
        - Normal
        - Gras
        - Demi-Gras
        - Maigre
        - Normal
        -
        - Extra-Gras
        - Italique
        - Oblique
        - Étroite
        - Large
        - Contour
      * - German
        - Standard
        - Fett
        - Halbfett
        - mager
        - mittel
   
          normal
        - Buchschrift
        - Schwarz
        - Kursiv
        - schräg
        - schmal
        - breit
        - Kontur
      * - Greek
        - κανονική
        - έντονη
        - ηµιέντονη
        - λεπτή
        - µεσαία
        - ßιßλίου
        - µαύρα
        - Λειψίας
        - πλάγια
        - πυκνή
        - αραιή
        - περιγράμματος
      * - Hungarian
        - Normàl
        - Kövér
        - FélkövérKövér
        - Világos
        - Közepes
        - Sötétes
        - Fekete
        - Dőlt
        - Döntött
        - Keskeny
        - Széles
        - Kontúros
      * - Italian
        - Normale
        - Nero
        - Neretto
        - Chiaro
        - Medio
        - Libro
        - ExtraNero
        - Corsivo
        - Obliquo
        - Condensato
        - Allargato
        -
      * - Norwegian
        - Vanlig
        - Halvfet
        -
        - Mager
        -
        -
        - Fet
        - Kursiv
        -
        - Smal
        - Sperret
        -
      * - Polish
        - :small:`odmiana` podstawowa
        - :small:`odmiana` pogrubiona
        - :small:`odmiana` półgruba
        - :small:`odmiana` bardzo cienka
        - :small:`odmiana` zwykła
        - :small:`odmiana` zwykła
        - :small:`odmiana` bardzo gruba
        - :small:`odmiana` pochyła
        - :small:`odmiana` pochyła
        - :small:`odmiana` wąska
        - :small:`odmiana` szeroka
        - :small:`odmiana` konturowa
      * - Russian
        - Обычный
        - Обычный
        - Полужирный
        - Светлый
        -
        -
        - Чёрный
        - Курсив
        - Наклон
        - Узкий
        - Широкий
        -
      * - Spanish
        - Normal
        - Negrita
        -
        - Fina
        -
        -
        - Supernegra
        - Cursiva
        -
        - Condensada
        - Ampliada
        -
      * - Swedish
        - Mager
        - Fet
        -
        - Extrafin
        -
        -
        - Extrafet
        - Kursiv
        -
        - Smal
        - Bred
        -
      * - Vietnamese
        - Chuẩn
        - Đậm
        - Nửa đậm
        - Nhẹ
        - Vừa
        - Sách
        - Đen
        - Nghiêng
        - Xiên
        - Hẹp
        - Rộng
        - Nét ngoài

(Any help in expanding/correcting the above table would be greatly appreciated).
For more information on translating fontforge see the
:doc:`UI Translation page <uitranslationnotes>`.

When you create a Style entry for an language in the
:ref:`Element->Font Info->TTF Name <fontinfo.TTF-Names>` dialog, FontForge will
attempt to translate the American English style into something appropriate for
that language. It understands languages in the above table, but not others so it
won't always work.

So if your style in American English is "BoldItalic" then after you create the
appropriate strings FontForge will default to "GrasItalique" for French,
"FettKursiv" for German, "ОбычныйКурсив" for Russian and "NigritaCursiva" for
Spanish.